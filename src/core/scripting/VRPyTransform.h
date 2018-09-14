#ifndef VRPyTransform_H_INCLUDED
#define VRPyTransform_H_INCLUDED

#include "VRPyObject.h"
#include "core/objects/VRTransform.h"
#include "core/objects/geometry/VRPhysics.h"

struct VRPyCollision : VRPyBaseT<OSG::VRCollision> {
    static PyMethodDef methods[];
};

struct VRPyTransform : VRPyBaseT<OSG::VRTransform> {
    static PyMethodDef methods[];
    static PyObject* fromSharedPtr(OSG::VRTransformPtr obj);
};

#endif // VRPyTransform_H_INCLUDED
