#ifndef VRPYNAVIGATOR_H_INCLUDED
#define VRPYNAVIGATOR_H_INCLUDED

#include "VRPyObject.h"
#include "core/navigation/VRNavigator.h"

struct VRPyNavigator : VRPyBaseT<OSG::VRNavigator> {
    static PyMethodDef methods[];

    static PyObject* getPreset(VRPyNavigator* self, PyObject* args);
};

#endif // VRPYNAVIGATOR_H_INCLUDED
