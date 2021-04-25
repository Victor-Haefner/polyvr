#ifndef VRPYENGINEERING_H_INCLUDED
#define VRPYENGINEERING_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRRobotArm.h"
#include "VRPipeSystem.h"
#include "VRNumberingEngine.h"
#include "Space/VRRocketExhaust.h"

struct VRPyNumberingEngine : VRPyBaseT<OSG::VRNumberingEngine> {
    static PyMethodDef methods[];
};

struct VRPyRobotArm : VRPyBaseT<OSG::VRRobotArm> {
    static PyMethodDef methods[];
};

struct VRPyPipeSystem : VRPyBaseT<OSG::VRPipeSystem> {
    static PyMethodDef methods[];
};

struct VRPyRocketExhaust : VRPyBaseT<OSG::VRRocketExhaust> {
    static PyMethodDef methods[];
};

#endif //VRPYENGINEERING_H_INCLUDED
