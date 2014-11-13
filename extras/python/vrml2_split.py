#!/bin/python

path = 'machine'

f = open(path+'.wrl', 'r')

N = 3000
i = 0
k = N
out = None
for l in f:
	if l.startswith('Transform'):
		if k == N:
			out = open('split/'+path+str(i)+'.wrl', 'w')
			out.write('#VRML V2.0 utf8\n')
			k = 0
		i += 1
		k += 1
	
	if out:
		out.write(l)


f.close()
print 'done'
