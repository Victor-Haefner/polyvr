#include "VRAxleSegmentation.h"
#include "core/utils/toString.h"

using namespace OSG;

VRAxleSegmentation::VRAxleSegmentation() {}
VRAxleSegmentation::~VRAxleSegmentation() {}

VRAxleSegmentationPtr VRAxleSegmentation::create() { return VRAxleSegmentationPtr(new VRAxleSegmentation()); }


void VRAxleSegmentation::analyse(VRObjectPtr o) {
    obj = o;
}



/**

    <Script base_name="axleAnalysis" group="kinematics" name_space="__script__" name_suffix="0" persistency="666" server="server1" type="Python">
      <core>

	import VR, random, time
	from VR.Math import Vec3

	#import pylab as plt
	import numpy, scipy.optimize

	if hasattr(VR, 'visualsT'): VR.visualsT.destroy()
	VR.visualsT = VR.Transform('visualsT')
	VR.scene.addChild(VR.visualsT)
	visuals = VR.AnalyticGeometry()
	VR.visualsT.addChild(visuals)

	pi = 3.14159265359

	class axleVertex:
		def __init__(self, v):
			self.vertex = v
			self.profileCoords = Vec3([0,0,0])
			self.plane = -1
			self.radius = 0

		def computeRadius(self):
			p = self.vertex
			p = p-d*d.dot(p)
			return p.length()

		def computeProfile(self):
			o = getOrthogonal(d)
			o2 = o.cross(d)
			p = self.vertex

			p = p-o*o.dot(p)
			x = d.dot(p)
			y = o2.dot(p)
			return (x,y)

		def computeAndSetAttributes(self):
			self.profileCoords = self.computeProfile()
			self.radius = self.computeRadius()
			return

		def setPlaneIndex(self, i):
			self.plane = i
			return

	axleVertices = {} # key is vertex ID (corresponds to position in geometry vertex array)


	def same(x,y):
		return abs(x-y) &lt; 1


	def getOrthogonal(v): #TODO improve calculation
		if abs(v[0]-v[1]) &lt; 0.1:
			return Vec3([v[0],-v[2],v[1]])
		return Vec3([v[1],-v[0],v[2]])

	if not obj:
		obj = VR.machine.findAll('4507 008 11 Antriebswelle KV-120')[0]

	# axle direction
	d = Vec3(obj.getEntity().getVector('axis')).normalize()

	# gear vertices
	geos = obj.getChildren(True, 'Geometry', True)

	pos = []
	for g in geos:
		pos += g.getPositions()
		g.makeUnique()

	for p in pos:
		v = axleVertex(p)
		v.computeAndSetAttributes()
		axleVertices[p] = v


	def getProfile():
		p = []
		for g in axleVertices.values():
			p += [g.profileCoords]
		return p

	def getVerticesOnSamePlane():
		p = {}
		a = []
		x = 0
		vertices = sorted(axleVertices.values(), key=lambda v: v.profileCoords[0])

		size = len(vertices)
		for i in range(0, size-1):
			a += [vertices[i]]
			vertices[i].setPlaneIndex(x)

			b1 = same(vertices[i].profileCoords[0], vertices[i+1].profileCoords[0])
			if not (b1):
				p[x] = a
				a = []
				x += 1
		lastVertex = vertices[size-1]
		a += [lastVertex]
		lastVertex.setPlaneIndex(x)
		p[x] = a #add last list
		return p

	def getMinMaxRadius(vertexList):
		rmin = 1e6
		rmax = 0
		for v in vertexList:
			rmin = min(rmin,v.radius)
			rmax = max(rmax,v.radius)
		return rmin, rmax

	def getMinMaxProfileHeight(vertexList): # y value
		pmin = 1e6
		pmax = 0
		for v in vertexList:
			pmin = min(pmin,v.profileCoords[1])
			pmax = max(pmax,v.profileCoords[1])
		return pmin, pmax

	def getMinMaxProfileDist(vertexList): #x value
		pmin = 1e6
		pmax = 0
		for v in vertexList:
			pmin = min(pmin,v.profileCoords[0])
			pmax = max(pmax,v.profileCoords[0])
		return pmin, pmax

	def dictToList(myDict):
		myList = []
		for e in myDict.values():
			myList.extend(e)
		return myList

	def calcOffset(vertexList):
		pmin, pmax = getMinMaxProfileDist(vertexList)
		poffset = pmin + (pmax - pmin)/2

		bb = obj.getWorldBoundingbox()
		p = bb.center()

		po = obj.getWorldPose()
		pb = po.mult(p)

		po.setPos([0,0,0])
		dL = po.mult(d)

		offset = pb +  dL*poffset
		return Vec3(offset)

	def calcWidth(vertexList):
		pmin, pmax = getMinMaxProfileDist(vertexList)
		w = pmax - pmin
		return w

	def getTotalLength():
		pmin = 1e6
		pmax = 0
		for v in axleVertices.values():
			pmin = min(pmin,v.profileCoords[0])
			pmax = max(pmax,v.profileCoords[0])
		return pmax - pmin

	def calcAxleParams(vertices):
		radius = getMinMaxRadius(vertices)[1]
		totalLength = getTotalLength()
		offset = calcOffset(vertices)

		if 0:
			print 'axle:', obj.getName()
			print 'totalLength:', totalLength
			print ' offset:', offset
		return [radius,totalLength,offset]


	def showProfile(vertices):
		#P = getProfile()
		P = [g.profileCoords for g in vertices]
		if 0:

			tt = numpy.array([ x for x,y in P ])
			yy = numpy.array([ y for x,y in P ])

			plt.plot(tt, yy, "ok", label="y", linewidth=2)
			plt.legend(loc="best")
			plt.show()
	###################################################


	planes = getVerticesOnSamePlane()
	if 0:
		print obj.getBaseName()
		print '  number of planes:', len(planes)

	result = {}
	i = 0

	for m in planes:

		vertices = planes[m]
		#showProfile(vertices)

		res = calcAxleParams(vertices)
		result[i] = res
		i += 1

	return result


*/
