#ifndef VRPYPATCHSELECTION_H_INCLUDED
#define VRPYPATCHSELECTION_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/selection/VRPatchSelection.h"

struct VRPyPatchSelection : VRPyBaseT<OSG::VRPatchSelection> {
    static PyMethodDef methods[];

    static PyObject* select(VRPyPatchSelection* self, PyObject* args);
};

#endif // VRPYPATCHSELECTION_H_INCLUDED
