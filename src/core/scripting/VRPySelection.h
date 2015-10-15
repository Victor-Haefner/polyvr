#ifndef VRPYSELECTION_H_INCLUDED
#define VRPYSELECTION_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/selection/VRSelection.h"

struct VRPySelection : VRPyBaseT<OSG::VRSelection> {
    static PyMethodDef methods[];

    static PyObject* add(VRPySelection* self, PyObject* args, PyObject* kwargs);
    static PyObject* sub(VRPySelection* self, PyObject* args, PyObject* kwargs);
    static PyObject* append(VRPySelection* self, PyObject* args);
    static PyObject* clear(VRPySelection* self);

    static PyObject* getSelected(VRPySelection* self);
    static PyObject* getPartialSelected(VRPySelection* self);
    static PyObject* getSubselection(VRPySelection* self, PyObject* args);
};

#endif // VRPYSELECTION_H_INCLUDED
