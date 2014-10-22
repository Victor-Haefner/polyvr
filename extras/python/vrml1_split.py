#!/bin/python

path = 'gabler_factory'

f = open(path+'.wrl', 'r')

N = 1000
i = 0
k = N
out = None
for l in f:
	if l.startswith('Separator'):
		if k == N:
			out = open('split/'+path+str(i)+'.wrl', 'w')
			out.write('#VRML V1.0 ascii\n')
			k = 0
		i += 1
		k += 1
	
	if out:
		out.write(l)


f.close()
print 'done'
