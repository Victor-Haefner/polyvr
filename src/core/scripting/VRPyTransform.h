#ifndef VRPyTransform_H_INCLUDED
#define VRPyTransform_H_INCLUDED

#include "VRPyObject.h"
#include "core/objects/VRTransform.h"

struct VRPyTransform : VRPyBaseT<OSG::VRTransform> {
    static PyMemberDef members[];
    static PyMethodDef methods[];

    static PyObject* setIdentity(VRPyTransform* self);
    static PyObject* translate(VRPyTransform* self, PyObject* args);
    static PyObject* move(VRPyTransform* self, PyObject* args);
    static PyObject* rotate(VRPyTransform* self, PyObject* args);

    static PyObject* getWFrom(VRPyTransform* self);
    static PyObject* getFrom(VRPyTransform* self);
    static PyObject* getAt(VRPyTransform* self);
    static PyObject* getDir(VRPyTransform* self);
    static PyObject* getUp(VRPyTransform* self);
    static PyObject* getScale(VRPyTransform* self);

    static PyObject* setPose(VRPyTransform* self, PyObject* args);
    static PyObject* setWFrom(VRPyTransform* self, PyObject* args);
    static PyObject* setFrom(VRPyTransform* self, PyObject* args);
    static PyObject* setAt(VRPyTransform* self, PyObject* args);
    static PyObject* setDir(VRPyTransform* self, PyObject* args);
    static PyObject* setUp(VRPyTransform* self, PyObject* args);
    static PyObject* setScale(VRPyTransform* self, PyObject* args);

    static PyObject* duplicate(VRPyTransform* self);
    static PyObject* physicalize(VRPyTransform* self, PyObject *args);
    static PyObject* setPhysicsConstraintTo(VRPyTransform* self, PyObject *args);
    static PyObject* setMass(VRPyTransform* self, PyObject *args);
    static PyObject* setCollisionMargin(VRPyTransform* self, PyObject *args);
    static PyObject* setCollisionGroup(VRPyTransform* self, PyObject *args);
    static PyObject* setCollisionMask(VRPyTransform* self, PyObject *args);
    static PyObject* setPhysicsActivationMode(VRPyTransform* self, PyObject *args);
    static PyObject* applyImpulse(VRPyTransform* self, PyObject *args);

    //physics data
    static PyObject* getPhysicsData(VRPyTransform* self);


    static PyObject* setPickable(VRPyTransform* self, PyObject* args);
    static PyObject* setPlaneConstraints(VRPyTransform* self, PyObject* args);
    static PyObject* setAxisConstraints(VRPyTransform* self, PyObject* args);
    static PyObject* setRotationConstraints(VRPyTransform* self, PyObject* args);

    static PyObject* animate(VRPyTransform* self, PyObject* args);
    static PyObject* animationStop(VRPyTransform* self);
};

#endif // VRPyTransform_H_INCLUDED
