#ifndef VRPYTEXTURERENDERER_H_INCLUDED
#define VRPYTEXTURERENDERER_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/VRTextureRenderer.h"

struct VRPyTextureRenderer : VRPyBaseT<OSG::VRTextureRenderer> {
    static PyMethodDef methods[];

    static PyObject* setup(VRPyTextureRenderer* self, PyObject* args);
    static PyObject* setActive(VRPyTextureRenderer* self, PyObject* args);
    static PyObject* getMaterial(VRPyTextureRenderer* self);
};

#endif // VRPYTEXTURERENDERER_H_INCLUDED
