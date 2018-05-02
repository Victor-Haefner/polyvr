#ifndef VRPYPROCESS_H_INCLUDED
#define VRPYPROCESS_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRProcess.h"
#include "VRProcessLayout.h"
#include "VRProcessEngine.h"

struct VRPyProcess : VRPyBaseT<OSG::VRProcess> {
    static PyMethodDef methods[];
};

struct VRPyProcessNode : VRPyBaseT<OSG::VRProcessNode> {
    static PyMethodDef methods[];
};

struct VRPyProcessDiagram : VRPyBaseT<OSG::VRProcessDiagram> {
    static PyMethodDef methods[];
};

struct VRPyProcessLayout : VRPyBaseT<OSG::VRProcessLayout> {
    static PyMethodDef methods[];
};

struct VRPyProcessEngine : VRPyBaseT<OSG::VRProcessEngine> {
    static PyMethodDef methods[];
};

#endif // VRPYPROCESS_H_INCLUDED
