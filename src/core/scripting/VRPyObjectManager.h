#ifndef VRPYOBJECTMANAGER_H_INCLUDED
#define VRPYOBJECTMANAGER_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "core/scene/VRObjectManager.h"

struct VRPyObjectManager : VRPyBaseT<OSG::VRObjectManager> {
    static PyMethodDef methods[];
};

#endif // VRPYOBJECTMANAGER_H_INCLUDED
