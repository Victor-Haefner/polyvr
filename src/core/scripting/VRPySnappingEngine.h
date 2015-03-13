#ifndef VRPYSNAPPINGENGINE_H_INCLUDED
#define VRPYSNAPPINGENGINE_H_INCLUDED

#include "VRPyBase.h"
#include "core/tools/VRSnappingEngine.h"

struct VRPySnappingEngine : VRPyBaseT<OSG::VRSnappingEngine> {
    static PyMethodDef methods[];

    static PyObject* addObject(VRPySnappingEngine* self, PyObject* args);
    static PyObject* addTree(VRPySnappingEngine* self, PyObject* args);
    static PyObject* setPreset(VRPySnappingEngine* self, PyObject* args);
};

#endif // VRPYSNAPPINGENGINE_H_INCLUDED
