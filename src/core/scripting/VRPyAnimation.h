#ifndef VRPYANIMATION_H_INCLUDED
#define VRPYANIMATION_H_INCLUDED

#include "core/math/path.h"
#include "core/scene/VRAnimationManager.h"
#include "VRPyBase.h"

struct VRPyAnimation : public VRPyBaseT<OSG::VRAnimationManager> {
    static PyMemberDef members[];
    static PyMethodDef methods[];

    static PyObject* setPath(VRPyAnimation* self, PyObject *args);
	static PyObject* setTarget(VRPyAnimation* self, PyObject *args);

	static PyObject* set(VRPyAnimation* self, PyObject *args);
	static PyObject* play(VRPyAnimation* self);

	static PyObject* stop(VRPyAnimation* self);
	// move in a direction
	static PyObject* move(VRPyAnimation* self, PyObject *args);
    // return the current step
	static PyObject* get(VRPyAnimation* self);
    static PyObject* setLoop(VRPyAnimation* self, PyObject *args);
	// set speed curve
	static PyObject* setRamp(VRPyAnimation* self, PyObject *args);
};

#endif // VRPYANIMATION_H_INCLUDED
