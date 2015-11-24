#ifndef VRPYONTOLOGY_H_INCLUDED
#define VRPYONTOLOGY_H_INCLUDED

#include "VROntology.h"
#include "core/scripting/VRPyBase.h"

struct VRPyOntology : VRPyBaseT<VROntology> {
    static PyMethodDef methods[];

    static PyObject* open(VRPyOntology* self, PyObject* args);
};

#endif // VRPYONTOLOGY_H_INCLUDED
