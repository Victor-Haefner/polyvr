#ifndef VRPYRENDERING_H_INCLUDED
#define VRPYRENDERING_H_INCLUDED

#include "VRPyObject.h"
#include "core/scene/rendering/VRRenderStudio.h"
#include "core/scene/rendering/VRRenderManager.h"

namespace OSG {
    typedef VRRenderManager VRRendering;
    typedef VRRenderManagerPtr VRRenderingPtr;
}

struct VRPyRenderStudio : VRPyBaseT<OSG::VRRenderStudio> {
    static PyMethodDef methods[];
};

struct VRPyRendering : VRPyBaseT<OSG::VRRendering> {
    static PyMethodDef methods[];
};

#endif // VRPYRENDERING_H_INCLUDED
