#ifndef VRPYSELECTOR_H_INCLUDED
#define VRPYSELECTOR_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/selection/VRSelector.h"

struct VRPySelector : VRPyBaseT<OSG::VRSelector> {
    static PyMethodDef methods[];

    static PyObject* setColor(VRPySelector* self, PyObject* args);
    static PyObject* select(VRPySelector* self, PyObject* args);
    static PyObject* update(VRPySelector* self);
    static PyObject* set(VRPySelector* self, PyObject* args);
    static PyObject* clear(VRPySelector* self);
    static PyObject* getSelection(VRPySelector* self);
};

#endif // VRPYSELECTOR_H_INCLUDED
