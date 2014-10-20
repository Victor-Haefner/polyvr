#ifndef VRPyColorChooser_H_INCLUDED
#define VRPyColorChooser_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/VRColorChooser.h"

struct VRPyColorChooser : VRPyBaseT<OSG::VRColorChooser> {
    static PyMethodDef methods[];

    static PyObject* setGeometry(VRPyColorChooser* self, PyObject* args);
    static PyObject* setColor(VRPyColorChooser* self, PyObject* args);
    static PyObject* getColor(VRPyColorChooser* self);
    static PyObject* getLastColor(VRPyColorChooser* self);
    static PyObject* resolve(VRPyColorChooser* self, PyObject* args);
};

#endif // VRPyColorChooser_H_INCLUDED
