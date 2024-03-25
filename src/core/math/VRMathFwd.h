#ifndef VRMATHFWD_H_INCLUDED
#define VRMATHFWD_H_INCLUDED


#include "core/utils/VRFwdDeclTemplate.h"
#include <map>
#include <string>
#include <Python.h>

namespace OSG {

ptrFwd(Boundingbox);
ptrFwd(Path);
ptrFwd(Datarow);
ptrFwd(Pose);
ptrFwd(VRPolygon);
ptrFwd(Frustum);
ptrFwd(Patch);
ptrFwd(Graph);
ptrFwd(Partitiontree);
//ptrFwd(PartitiontreeNode);
ptrFwd(Quadtree);
ptrFwd(QuadtreeNode);
//ptrFwd(Octree);
//ptrFwd(OctreeNode);
ptrFwd(Triangulator);
ptrFwd(VRKinematics);
ptrFwd(VRKabschAlgorithm);
ptrFwd(FABRIK);
ptrFwd(Expression);
ptrFwd(MathExpression);
ptrFwd(TSDF);
ptrFwd(PCA);
ptrFwd(PID);
ptrFwd(SineFit);

template<class T>
class OctreeNode;

template<class T>
class Octree;

ptrTemplateFwd( VRStateMachine, VRStateMachinePy, PyObject* );
typedef std::map<std::string, std::string> strMap;
ptrTemplateFwd( VRStateMachine, VRStateMachineMap, strMap );

}

#endif // VRMATHFWD_H_INCLUDED
