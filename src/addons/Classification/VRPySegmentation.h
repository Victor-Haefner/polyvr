#ifndef VRPYSEGMENTATION_H_INCLUDED
#define VRPYSEGMENTATION_H_INCLUDED

#include "VRSegmentation.h"
#include "core/scripting/VRPyBase.h"

struct VRPySegmentation : VRPyBaseT<OSG::VRSegmentation> {
    static PyMethodDef methods[];

    static PyObject* extractPatches(VRPySegmentation* self, PyObject* args);
};

#endif // VRPYSEGMENTATION_H_INCLUDED
