#ifndef VRPYCAVEKEEPER_H_INCLUDED
#define VRPYCAVEKEEPER_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "core/objects/object/VRObject.h"
#include "CaveKeeper.h"

struct VRPyCaveKeeper : VRPyBaseT<OSG::CaveKeeper> {
    static PyMemberDef members[];
    static PyMethodDef methods[];

    static PyObject* initWorld(VRPyCaveKeeper* self, PyObject* args);
    static PyObject* update(VRPyCaveKeeper* self, PyObject* args);
    static PyObject* dig(VRPyCaveKeeper* self, PyObject* args);
    static PyObject* place(VRPyCaveKeeper* self, PyObject* args);
};

#endif // VRPYCAVEKEEPER_H_INCLUDED
