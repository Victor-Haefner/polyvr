#ifndef VRPYTEXTURERENDERER_H_INCLUDED
#define VRPYTEXTURERENDERER_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/VRTextureRenderer.h"

struct VRPyTextureRenderer : VRPyBaseT<OSG::VRTextureRenderer> {
    static PyMethodDef methods[];
};

#endif // VRPYTEXTURERENDERER_H_INCLUDED
