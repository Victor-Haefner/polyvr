#ifndef VRPyCEF_H_INCLUDED
#define VRPyCEF_H_INCLUDED

#include "CEF.h"
#include "../../core/scripting/VRPyBase.h"

namespace OSG{ class VRMaterial; }

struct VRPyCEF : VRPyBaseT<CEF> {
    static PyMethodDef methods[];

    static PyObject* open(VRPyCEF* self, PyObject* args);
    static PyObject* setMaterial(VRPyCEF* self, PyObject* args);
    static PyObject* addMouse(VRPyCEF* self, PyObject* args);
    static PyObject* addKeyboard(VRPyCEF* self, PyObject* args);
    static PyObject* setResolution(VRPyCEF* self, PyObject* args);
    static PyObject* setAspectRatio(VRPyCEF* self, PyObject* args);
};

#endif // VRPyCEF_H_INCLUDED
