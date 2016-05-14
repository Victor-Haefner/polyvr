#ifndef VRPYCAVEKEEPER_H_INCLUDED
#define VRPYCAVEKEEPER_H_INCLUDED

#include "core/scripting/VRPyBase.h"
//#include "core/objects/object/VRObject.h"
#include "CaveKeeper.h"

struct VRPyCaveKeeper : VRPyBaseT<OSG::CaveKeeper> {
    static PyMethodDef methods[];

    static PyObject* initWorld(VRPyCaveKeeper* self, PyObject* args);
    static PyObject* intersect(VRPyCaveKeeper* self, PyObject* args);
    static PyObject* addBlock(VRPyCaveKeeper* self, PyObject* args);
    static PyObject* remBlock(VRPyCaveKeeper* self, PyObject* args);
    static PyObject* addObject(VRPyCaveKeeper* self, PyObject* args);
};

#endif // VRPYCAVEKEEPER_H_INCLUDED
