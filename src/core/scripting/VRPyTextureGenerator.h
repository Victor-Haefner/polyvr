#ifndef VRPYTEXTUREGENERATOR_H_INCLUDED
#define VRPYTEXTUREGENERATOR_H_INCLUDED

#include "VRPyObject.h"
#include "core/objects/material/VRTextureGenerator.h"

struct VRPyTextureGenerator : VRPyBaseT<OSG::VRTextureGenerator> {
    static PyMethodDef methods[];

    static PyObject* add(VRPyTextureGenerator* self, PyObject* args);
    static PyObject* setSize(VRPyTextureGenerator* self, PyObject* args);
    static PyObject* compose(VRPyTextureGenerator* self, PyObject* args);
};

#endif // VRPYTEXTUREGENERATOR_H_INCLUDED
