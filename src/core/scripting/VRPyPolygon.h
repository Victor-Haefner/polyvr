#ifndef VRPYPOLYGON_H_INCLUDED
#define VRPYPOLYGON_H_INCLUDED

#include "core/math/polygon.h"
#include "core/math/partitioning/frustum.h"
#include "core/scripting/VRPyBase.h"

struct VRPyPolygon : VRPyBaseT<OSG::VRPolygon> {
    static PyMethodDef methods[];
};

struct VRPyFrustum : VRPyBaseT<OSG::Frustum> {
    static PyMethodDef methods[];
};

#endif // VRPYPOLYGON_H_INCLUDED
