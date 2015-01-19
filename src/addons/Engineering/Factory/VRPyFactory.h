#ifndef VRPYFACTORY_H_INCLUDED
#define VRPYFACTORY_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRFactory.h"

struct VRPyFactory : VRPyBaseT<OSG::VRFactory> {
    static PyMethodDef methods[];

    static PyObject* loadVRML(VRPyFactory* self, PyObject* args);
    static PyObject* setupLod(VRPyFactory* self, PyObject* args);
};

#endif // VRPYFACTORY_H_INCLUDED
