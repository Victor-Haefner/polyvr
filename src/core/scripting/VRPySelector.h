#ifndef VRPYSELECTOR_H_INCLUDED
#define VRPYSELECTOR_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/selection/VRSelector.h"

struct VRPySelector : VRPyBaseT<OSG::VRSelector> {
    static PyMethodDef methods[];

    static PyObject* setColor(VRPySelector* self, PyObject* args);
    static PyObject* select(VRPySelector* self, PyObject* args);
    static PyObject* deselect(VRPySelector* self);
    static PyObject* getSelection(VRPySelector* self);
    static PyObject* getSubselection(VRPySelector* self);
    static PyObject* addSubselection(VRPySelector* self, PyObject* args);
    static PyObject* remSubselection(VRPySelector* self, PyObject* args);
    static PyObject* clearSubselection(VRPySelector* self);
};

#endif // VRPYSELECTOR_H_INCLUDED
