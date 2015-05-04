#ifndef VRPYANIMATION_H_INCLUDED
#define VRPYANIMATION_H_INCLUDED

#include "core/math/path.h"
#include "core/scene/VRAnimationManager.h"
#include "VRPyBase.h"

struct VRPyAnimation : public VRPyBaseT<OSG::VRAnimation> {
    static PyMethodDef methods[];

    static PyObject* start(VRPyAnimation* self);
	static PyObject* stop(VRPyAnimation* self);
	static PyObject* isActive(VRPyAnimation* self);
};

#endif // VRPYANIMATION_H_INCLUDED
