#ifndef VRPYOBJECT_H_INCLUDED
#define VRPYOBJECT_H_INCLUDED

#include "core/objects/object/VRObject.h"
#include "VRPyBase.h"

struct VRPyObject : VRPyBaseT<OSG::VRObject> {
    static PyMemberDef members[];
    static PyMethodDef methods[];

    static PyObject* getName(VRPyObject* self);
    static PyObject* setName(VRPyObject* self, PyObject* args);
    static PyObject* addChild(VRPyObject* self, PyObject* args, PyObject *kwds);
    static PyObject* switchParent(VRPyObject* self, PyObject* args, PyObject *kwds);
    static PyObject* destroy(VRPyObject* self);
    static PyObject* duplicate(VRPyObject* self);
    static PyObject* hide(VRPyObject* self);
    static PyObject* show(VRPyObject* self);
    static PyObject* isVisible(VRPyObject* self);
    static PyObject* setVisible(VRPyObject* self, PyObject* args);
    static PyObject* getType(VRPyObject* self);
    static PyObject* getChild(VRPyObject* self, PyObject* args);
    static PyObject* getChildren(VRPyObject* self, PyObject* args);
    static PyObject* getParent(VRPyObject* self);
    static PyObject* find(VRPyObject* self, PyObject* args);
    static PyObject* isPickable(VRPyObject* self);
    static PyObject* setPickable(VRPyObject* self, PyObject* args);
    static PyObject* printOSG(VRPyObject* self);
};

#endif // VRPYOBJECT_H_INCLUDED
