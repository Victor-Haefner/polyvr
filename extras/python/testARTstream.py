#!/usr/bin/python

import socket, math
from time import sleep, time
from random import random

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

class body:
	def __init__(self, ID):
		self.ID = ID
		self.loc = [0.0,0.0,-2000] # in mm
		self.rot = [1.0,0.0,0.0, 0.0,1.0,0.0, 0.0,0.0,1.0]

bodies = []
bodies.append( body(0) )
bodies.append( body(1) )
bodies.append( body(2) )

def modifyData():
	t = time()*0.3
	for b in bodies: 
		b.loc[0] = math.cos(t)*1000
		b.loc[1] = b.ID*200

def buildFrame():
	Nb = len(bodies)
	data = 'fr 0\n'
	data += 'ts 0\n'
	data += '6dcal '+str(Nb)+'\n'
	data += '6d '+str(Nb)
	for i in range(Nb):
		b = bodies[i]
		data += ' [' + str(i) + ' 1.0] ['
		for f in b.loc: data += ' ' + str(f)
		data += '] ['
		for f in b.rot: data += ' ' + str(f)
		data += ']'
	data += '\n'
	return data

while True:
	modifyData()
	data = buildFrame()
	sock.sendto(data, ('localhost', 5000))
	sleep(0.05)
