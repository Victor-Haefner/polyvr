#ifndef VRPYANNOTATIONENGINE_H_INCLUDED
#define VRPYANNOTATIONENGINE_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "core/tools/VRAnnotationEngine.h"

struct VRPyAnnotationEngine : VRPyBaseT<OSG::VRAnnotationEngine> {
    static PyMethodDef methods[];

    static PyObject* set(VRPyAnnotationEngine* self, PyObject* args);
    static PyObject* clear(VRPyAnnotationEngine* self);
};

#endif // VRPYANNOTATIONENGINE_H_INCLUDED
