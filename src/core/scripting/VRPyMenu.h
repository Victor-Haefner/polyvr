#ifndef VRPYMENU_H_INCLUDED
#define VRPYMENU_H_INCLUDED

#include "VRPyBase.h"
#include "core/tools/VRMenu.h"

struct VRPyMenu : VRPyBaseT<OSG::VRMenu> {
    static PyMethodDef methods[];

    static PyObject* append(VRPyMenu* self, PyObject* args);
    static PyObject* setLeafType(VRPyMenu* self, PyObject* args);
    static PyObject* setLayout(VRPyMenu* self, PyObject* args);
    static PyObject* open(VRPyMenu* self);
    static PyObject* close(VRPyMenu* self);
};

#endif // VRPYMENU_H_INCLUDED
