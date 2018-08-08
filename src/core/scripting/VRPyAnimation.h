#ifndef VRPYANIMATION_H_INCLUDED
#define VRPYANIMATION_H_INCLUDED

#include "core/math/path.h"
#include "core/scene/VRAnimationManager.h"
#include "VRPyBase.h"

struct VRPyAnimation : public VRPyBaseT<OSG::VRAnimation> {
    static PyMethodDef methods[];
};

#endif // VRPYANIMATION_H_INCLUDED
