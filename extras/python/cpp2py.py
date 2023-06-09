#!/usr/bin/env python3


import sys


class State:
	def __init__(self, fOut):
		self.fOut = fOut
		self.depth = 0
		self.inStr = False
		self.inWhitespace = False
		self.data = ''
		
	def flush(self):
		if self.data.endswith(' :'): self.data = self.data[:-2] + ':'
		s = self.depth*'\t' + self.data + '\n'
		self.fOut.write(s)
		self.data = ''
		
	def append(self, c):
		if self.inWhitespace:
			self.data += ' '
			self.inWhitespace = False
		self.data += c

def handleChar(state, c):
	if c in ['\t', '\n', ' ']: 
		state.inWhitespace = True
	elif c == '{': 
		state.append(':')
		state.flush()
		state.depth += 1
	elif c == '}': 
		state.flush()
		state.depth -= 1
	elif c == ';': state.flush()
	else: state.append(c)

def handleLine(state, line):
	if line.startswith('#'): return
	state.inWhitespace = False
	line = line.replace('->', '.')
	line = line.replace('VR', 'VR.')
	line = line.replace('::create', '')
	line = line.replace('::', '.')
	line = line.replace('&&', 'and')
	line = line.replace('||', 'or')
	line = line.replace('++', '+=1')
	line = line.replace('Vec3d()', '[0,0,0]')
	line = line.replace('Vec3d(', '[')
	line = line.replace('public:', '')
	line = line.replace('private:', '')
	line = line.replace('protected:', '')
	line = line.replace('Ptr', '')
	line = line.replace('void ', 'def ')
	line = line.replace('auto ', '')
	for c in line: handleChar(state, c)


# process file
src = sys.argv[1]
dst = sys.argv[2]

print('convert', src, 'to', dst, '\n')


fOut = open(dst, 'w')
state = State(fOut)

fIn = open(src, 'r')
for line in fIn.readlines():
	line = handleLine(state, line.strip())
	
fIn.close()
fOut.close()
	
	
# read example
fRes = open(dst, 'r')
for i,line in enumerate(fRes.readlines()):
	if i < 30: print(line[:-1])



