#ifndef VRPYRENDERING_H_INCLUDED
#define VRPYRENDERING_H_INCLUDED

#include "VRPyObject.h"
#include "core/scene/VRRenderManager.h"

namespace OSG {
    typedef VRRenderManager VRRendering;
}

struct VRPyRendering : VRPyBaseT<OSG::VRRendering> {
    static PyMethodDef methods[];

    static PyObject* addStage(VRPyRendering* self, PyObject* args);
    static PyObject* setStageShader(VRPyRendering* self, PyObject* args);
    static PyObject* setStageActive(VRPyRendering* self, PyObject* args);
    static PyObject* addStageBuffer(VRPyRendering* self, PyObject* args);
    static PyObject* setStageParameter(VRPyRendering* self, PyObject* args);
};

#endif // VRPYRENDERING_H_INCLUDED
