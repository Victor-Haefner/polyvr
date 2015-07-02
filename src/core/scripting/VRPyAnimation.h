#ifndef VRPYANIMATION_H_INCLUDED
#define VRPYANIMATION_H_INCLUDED

#include "core/math/path.h"
#include "core/scene/VRAnimationManager.h"
#include "VRPyBase.h"

struct VRPyAnimation : public VRPyBaseT<OSG::VRAnimation> {
    static PyMethodDef methods[];

    static PyObject* start(VRPyAnimation* self, PyObject* args);
    static PyObject* setCallback(VRPyAnimation* self, PyObject* args);
	static PyObject* stop(VRPyAnimation* self);
	static PyObject* isActive(VRPyAnimation* self);
	static PyObject* setLoop(VRPyAnimation* self, PyObject* args);
	static PyObject* setDuration(VRPyAnimation* self, PyObject* args);
};

#endif // VRPYANIMATION_H_INCLUDED
