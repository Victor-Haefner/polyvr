#ifndef VRPYREALWORLD_H_INCLUDED
#define VRPYREALWORLD_H_INCLUDED

#include "../../core/scripting/VRPyBase.h"
#include "RealWorld.h"

struct VRPyRealWorld : VRPyBaseT<realworld::RealWorld> {
    static PyMemberDef members[];
    static PyMethodDef methods[];

    static PyObject* initWorld(VRPyRealWorld* self, PyObject* args);
    static PyObject* update(VRPyRealWorld* self, PyObject* args);
    static PyObject* physicalize(VRPyRealWorld* self, PyObject* args);
    static PyObject* enableModule(VRPyRealWorld* self, PyObject* args);
    static PyObject* disableModule(VRPyRealWorld* self, PyObject* args);
};

#endif // VRPYREALWORLD_H_INCLUDED
