#!/usr/bin/python

import os, fnmatch, subprocess
import numpy, Gnuplot # sudo apt install python-gnuplot
import datetime
from PIL import Image
from time import sleep

sourceDir = '../../src'
#sourceDir = 'testData'
resolution = '8000, 8000'


def add(L, F, R):
	if not F in L: L[F] = R+'/'+F
	else: print 'Warning!', F, 'allready present (', L[F], ',', R+'/'+F, ')'

def getIncludes(path):
	includes = []
	for line in open(path):
		if '#include' in line: includes.append(line)
	return includes

def getFile(include, includeGraph):
	for File in includeGraph:
		if File in include: return File
	#return include

def openImg(path):
	img = Image.open(path)
	img.show()

class Plot:
	def __init__(self):
		print 'init graph'
		self.g = Gnuplot.Gnuplot()
		self.g.title("PolyVR Code Analytics, Include Graph")

		self.g("set grid")
		self.g("set xtic 1")
		self.g("set ytic 1")
		#self.g('set terminal x11 size 2000, 1000')
		self.g('set terminal png size '+resolution)
		#self.g('set size 2,2')

		self.edges = []
		self.nodes = []
		print ' done'
		
	def draw(self):
		print 'draw'
		vectorsX0 = []
		vectorsY0 = []
		vectorsX = []
		vectorsY = []
		points = []

		for e in self.edges: 
			vectorsX0.append(e[1][0])
			vectorsY0.append(e[1][1])
			vectorsX.append(e[0][0]-e[1][0])
			vectorsY.append(e[0][1]-e[1][1])

		for n in self.nodes:
			points.append([n[0], n[1]])
			self.g('set label "'+n[2]+'" at '+str(n[0]-0.1)+','+str(n[1])+' font "Sans,10"')	

		d1 = Gnuplot.Data(vectorsX0, vectorsY0, vectorsX, vectorsY, title="", with_="vectors")
		d2 = Gnuplot.Data(points, title="", with_="points")

		self.g.plot(d1, d2)
		self.g.hardcopy (filename='/tmp/polyvr.png', terminal='png') # write last plot to another terminal
		sleep(7)
		openImg('/tmp/polyvr.png')
		input("Press Enter to continue...")
		print ' done'

	def addEdge(self, p0,p1):
		self.edges.append([p0,p1])

	def addNode(self, pos, name):
		self.nodes.append(pos+[name])

class Analytics:
	def __init__(self):
		self.headers = {}
		self.sources = {}
		self.positions = {}
		self.dependsGraph = {} # key is file name, value is list of files included
		self.includeGraph = {} # key is file name, value is list of files that include it
		self.layers = [[]]

	def traverseSources(self, sourceDir):
		for root, Dir, files in os.walk(sourceDir):
			for item in fnmatch.filter(files, "*.h"): add(self.headers, item, root)
			for item in fnmatch.filter(files, "*.cpp"): add(self.sources, item, root)

		print 'N headers:', len(self.headers)
		print 'N sources:', len(self.sources)

		# fill depends graph
		for name, path in self.headers.items(): self.dependsGraph[name] = getIncludes(path)
		for name, path in self.sources.items(): self.dependsGraph[name] = getIncludes(path)
		for name, path in self.headers.items(): self.includeGraph[name] = []

		for name, includes in self.dependsGraph.items():
			newIncludes = []
			for include in includes:
				f = getFile(include, self.headers)
				if f: newIncludes.append(f)
			self.dependsGraph[name] = newIncludes
		
		# fill include graph
		for name, includes in self.dependsGraph.items():
			for include in includes:
				if include in self.includeGraph:
					if not name in self.includeGraph[include]: self.includeGraph[include].append(name)


	def computeLayers(self):
		processedFiles = {}

		def getNewLayer(layer, layers):
			ref = layer[:]
			newLayer = []
			for f in layer:
				if not f in self.includeGraph: continue
				for includer in self.includeGraph[f]: 
					if includer in processedFiles: processedFiles[includer].remove(includer)
					newLayer.append(includer)
					processedFiles[includer] = newLayer
			if ref == newLayer: 
				del layers[-1]
				layers.append(newLayer)
				return []
			return newLayer

		def addNextLayer(layer, layers):
			if len(layers) > 20: return # safety check
			newLayer = getNewLayer(layer, layers)
			if len(newLayer) > 0:
				layers.append(newLayer)
				addNextLayer(newLayer, layers)

		for header in self.headers:
			if len(self.dependsGraph[header]) == 0 and len(self.includeGraph[header]) > 0:
				self.layers[0].append(header)

		addNextLayer(self.layers[0], self.layers)

	def computePositions(self):
		for j,layer in enumerate(self.layers):
			for i,f in enumerate(layer):
				if not f in self.positions: self.positions[f] = [j+0.1, -i-0.1]

def drawGraph():
	P = Plot()
	for label, pos in A.positions.items(): P.addNode(pos, label)
	for name, includes in A.dependsGraph.items():
		for include in includes:
			if name in A.positions and include in A.positions:
				P.addEdge(A.positions[name], A.positions[include])
	P.draw()

compilationTimes = {}
totalTime = 0
def getCompilationTimes():
	def runCompilation(h):
		with open("testMain.cpp") as f: lines = f.readlines()
		lines[0] = '#include "'+A.headers[h]+'"\n'
		with open("testMain.cpp", "w") as f: f.writelines(lines)

		cmd = "codeblocks -ni -ns --multiple-instance --target='Release' --rebuild PolyVR_18.04.cbp > /dev/null"
		ta = datetime.datetime.now()
		os.system(cmd)
		tb = datetime.datetime.now()
		return (tb - ta).total_seconds()

	i = 0
	tA = datetime.datetime.now()
	for header in A.headers:
		print 'compile header:', header,
		compilationTimes[header] = runCompilation(header)
		print compilationTimes[header]
		i += 1
		#if i == 5: break
	tB = datetime.datetime.now()
	return (tB - tA).total_seconds()
		

def countImpact(): # compute how many source files the header impacts
	def countIncludes(header, processed):
		processed.append(header)
		if header in A.includeGraph: 
			for f in A.includeGraph[header]:
				if not f in processed: countIncludes(f, processed)

	import random, operator
	Ndepends = {}
	for header in A.headers: 
		processed = []
		countIncludes(header, processed)
		Nc = 0
		Nh = 0
		for p in processed: 
			if p[-4:] == '.cpp': Nc+=1
			if p[-2:] == '.h': Nh+=1
		Ndepends[header] = (Nh, processed, Nc)
	Ndepends = sorted(Ndepends.items(), key=operator.itemgetter(1,0))
	for h in Ndepends:
		#print len(h[1][1]), h
		print h[0], ' '*(40-len(h[0])), len(A.includeGraph[h[0]]), '\t,', h[1][2], '\t/', len(A.sources), '\t,', h[1][0], '\t/', len(A.headers), '\t- ', 
		if h[0] in compilationTimes: print compilationTimes[h[0]], ' s'
		else: print ' '
	
	print 'total compilation time:', totalTime


A = Analytics()
A.traverseSources(sourceDir)
A.computeLayers()
A.computePositions()
totalTime = getCompilationTimes()
countImpact()
#drawGraph()















		
				

		


