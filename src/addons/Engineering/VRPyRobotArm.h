#ifndef VRPYROBOTARM_H_INCLUDED
#define VRPYROBOTARM_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRRobotArm.h"

struct VRPyRobotArm : VRPyBaseT<OSG::VRRobotArm> {
    static PyMethodDef methods[];
};

#endif // VRPYROBOTARM_H_INCLUDED
