#ifndef VRMATHFWD_H_INCLUDED
#define VRMATHFWD_H_INCLUDED


#include "core/utils/VRFwdDeclTemplate.h"
#include <map>
#include <string>
#include <Python.h>

namespace OSG {

ptrFwd(Boundingbox);
ptrFwd(Path);
ptrFwd(Pose);
ptrFwd(VRPolygon);
ptrFwd(Graph);
ptrFwd(Octree);
ptrFwd(Triangulator);
ptrFwd(VRKinematics);

ptrTemplateFwd( VRStateMachine, VRStateMachinePy, PyObject* );
typedef std::map<std::string, std::string> strMap;
ptrTemplateFwd( VRStateMachine, VRStateMachineMap, strMap );

}

#endif // VRMATHFWD_H_INCLUDED
