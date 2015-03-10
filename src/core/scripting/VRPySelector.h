#ifndef VRPYSELECTOR_H_INCLUDED
#define VRPYSELECTOR_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/VRSelector.h"

struct VRPySelector : VRPyBaseT<OSG::VRSelector> {
    static PyMethodDef methods[];

    static PyObject* setColor(VRPySelector* self, PyObject* args);
    static PyObject* select(VRPySelector* self, PyObject* args);
    static PyObject* deselect(VRPySelector* self);
    static PyObject* get(VRPySelector* self);
};

#endif // VRPYSELECTOR_H_INCLUDED
