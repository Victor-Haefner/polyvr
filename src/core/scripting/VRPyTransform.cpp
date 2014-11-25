#include "VRPyTransform.h"
#include "VRPyConstraint.h"
#include "VRPyPath.h"
#include "VRPyBaseT.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRConstraint.h"

template<> PyTypeObject VRPyBaseT<OSG::VRTransform>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Transform",             /*tp_name*/
    sizeof(VRPyTransform),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "VRTransform binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyTransform::methods,             /* tp_methods */
    VRPyTransform::members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_VRObjects,                 /* tp_new */
};

PyMemberDef VRPyTransform::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyTransform::methods[] = {
    {"setIdentity", (PyCFunction)VRPyTransform::setIdentity, METH_NOARGS, "Reset transformation to identity" },
    {"translate", (PyCFunction)VRPyTransform::translate, METH_VARARGS, "Translate the object along a vector - translate(xf,yf,zf)" },
    {"move", (PyCFunction)VRPyTransform::move, METH_VARARGS, "Move the object - move(d)" },
    {"rotate", (PyCFunction)VRPyTransform::rotate, METH_VARARGS, "Rotate the object around an axis - rotate(xf,yf,zf,af)" },
    {"getWorldFrom", (PyCFunction)VRPyTransform::getWFrom, METH_NOARGS, "Return the object's world position" },
    {"getFrom", (PyCFunction)VRPyTransform::getFrom, METH_NOARGS, "Return the object's from vector" },
    {"getAt", (PyCFunction)VRPyTransform::getAt, METH_NOARGS, "Return the object's at vector" },
    {"getDir", (PyCFunction)VRPyTransform::getDir, METH_NOARGS, "Return the object's dir vector" },
    {"getUp", (PyCFunction)VRPyTransform::getUp, METH_NOARGS, "Return the object's up vector" },
    {"getScale", (PyCFunction)VRPyTransform::getScale, METH_NOARGS, "Return the object's scale vector" },
    {"setWorldFrom", (PyCFunction)VRPyTransform::setWFrom, METH_VARARGS, "Set the object's world position" },
    {"setPose", (PyCFunction)VRPyTransform::setPose, METH_VARARGS, "Set the object's from dir and up vector" },
    {"setPosition", (PyCFunction)VRPyTransform::setFrom, METH_VARARGS, "Set the object's from vector" },
    {"setFrom", (PyCFunction)VRPyTransform::setFrom, METH_VARARGS, "Set the object's from vector" },
    {"setAt", (PyCFunction)VRPyTransform::setAt, METH_VARARGS, "Set the object's at vector" },
    {"setDir", (PyCFunction)VRPyTransform::setDir, METH_VARARGS, "Set the object's dir vector" },
    {"setUp", (PyCFunction)VRPyTransform::setUp, METH_VARARGS, "Set the object's up vector" },
    {"setScale", (PyCFunction)VRPyTransform::setScale, METH_VARARGS, "Set the object's scale vector" },
    {"setPickable", (PyCFunction)VRPyTransform::setPickable, METH_VARARGS, "Set the object pickable - setPickable(True/False)" },
    {"setPlaneConstraints", (PyCFunction)VRPyTransform::setPlaneConstraints, METH_VARARGS, "Constraint the object on a plane - setPlaneConstraints(nxf, nyf, nzf)" },
    {"setAxisConstraints", (PyCFunction)VRPyTransform::setAxisConstraints, METH_VARARGS, "Constraint the object on an axis - TODO -> to test, may work" },
    {"setRotationConstraints", (PyCFunction)VRPyTransform::setRotationConstraints, METH_VARARGS, "Constraint the object's rotation - setRotationConstraints(xi, yi, zi)" },
    {"duplicate", (PyCFunction)VRPyTransform::duplicate, METH_NOARGS, "duplicate transform" },
    {"physicalize", (PyCFunction)VRPyTransform::physicalize, METH_VARARGS, "physicalize subtree - physicalize( physicalized , dynamic , concave )" },
    {"attach", (PyCFunction)VRPyTransform::setPhysicsConstraintTo, METH_VARARGS, "create a constraint between this obejct and another - setPhysicsConstraintTo( Transform , Constraint )" },
    {"setMass", (PyCFunction)VRPyTransform::setMass, METH_VARARGS, "Set the mass of the physics object" },
    {"setCollisionMargin", (PyCFunction)VRPyTransform::setCollisionMargin, METH_VARARGS, "Set the collision margin of the physics object" },
    {"setCollisionGroup", (PyCFunction)VRPyTransform::setCollisionGroup, METH_VARARGS, "Set the collision group of the physics object" },
    {"setCollisionMask", (PyCFunction)VRPyTransform::setCollisionMask, METH_VARARGS, "Set the collision mask of the physics object" },
    {"applyImpulse", (PyCFunction)VRPyTransform::applyImpulse, METH_VARARGS, "Apply impulse on the physics object" },
    {"setPhysicsActivationMode", (PyCFunction)VRPyTransform::setPhysicsActivationMode, METH_VARARGS, "Set the physics activation mode of the physics object (normal:1 , no deactivation:4, stay deactivated: 5)" },
    {"animate", (PyCFunction)VRPyTransform::animate, METH_VARARGS, "Animate object (currently only with a path: animate(path, duration, redirect) )" },
    {"animationStop", (PyCFunction)VRPyTransform::animationStop, METH_NOARGS, "Stop any running animation of this object" },
    {"getPhysicsData", (PyCFunction)VRPyTransform::getPhysicsData, METH_NOARGS, "get Data to the physics" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyTransform::animationStop(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::animationStop, Object is invalid"); return NULL; }
    self->obj->stopAnimation();
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setIdentity(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setIdentity, Object is invalid"); return NULL; }
    self->obj->setMatrix(OSG::Matrix());
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::translate(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::translate, Object is invalid"); return NULL; }
    self->obj->translate( parseVec3f(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::move(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::move, Object is invalid"); return NULL; }
    float t = parseFloat(args);
    self->obj->move(t);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::rotate(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::rotate, Object is invalid"); return NULL; }
    OSG::Vec4f r = parseVec4f(args);

    OSG::Vec3f axis = OSG::Vec3f(r);
    float angle = r[3];

    OSG::VRTransform* e = (OSG::VRTransform*) self->obj;
    e->rotate(angle, axis);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::getFrom(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::getFrom, Object is invalid"); return NULL; }
    return toPyTuple(self->obj->getFrom());
}

PyObject* VRPyTransform::getWFrom(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::getWFrom, Object is invalid"); return NULL; }
    return toPyTuple(self->obj->getWorldPosition());
}

PyObject* VRPyTransform::getAt(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::getAt, Object is invalid"); return NULL; }
    return toPyTuple(self->obj->getAt());
}

PyObject* VRPyTransform::getDir(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::getDir, Object is invalid"); return NULL; }
    return toPyTuple(self->obj->getDir());
}

PyObject* VRPyTransform::getUp(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::getUp, Object is invalid"); return NULL; }
    return toPyTuple(self->obj->getUp());
}

PyObject* VRPyTransform::getScale(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::getScale, Object is invalid"); return NULL; }
    return toPyTuple(self->obj->getScale());
}

PyObject* VRPyTransform::setPose(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setPose, Object is invalid"); return NULL; }
    PyObject *fl, *dl, *ul;
    if (! PyArg_ParseTuple(args, "OOO", &fl, &dl, &ul)) return NULL;
    self->obj->setPose( parseVec3fList(fl), parseVec3fList(dl), parseVec3fList(ul));
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setFrom(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setFrom, Object is invalid"); return NULL; }
    OSG::Vec3f v = parseVec3f(args);
    self->obj->setFrom(v);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setWFrom(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setWFrom, Object is invalid"); return NULL; }
    OSG::Vec3f v = parseVec3f(args);
    self->obj->setWorldPosition(v);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setAt(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setAt, Object is invalid"); return NULL; }
    OSG::Vec3f v = parseVec3f(args);
    self->obj->setAt(v);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setDir(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setDir, Object is invalid"); return NULL; }
    OSG::Vec3f v = parseVec3f(args);
    self->obj->setDir(v);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setUp(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setUp, Object is invalid"); return NULL; }
    OSG::Vec3f v = parseVec3f(args);
    self->obj->setUp(v);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setScale(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setScale, Object is invalid"); return NULL; }
    OSG::Vec3f v = parseVec3f(args);
    self->obj->setScale(v);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setPickable(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setPickable, Object is invalid"); return NULL; }
    bool b = parseBool(args);
    self->obj->setPickable(b);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setPlaneConstraints(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setPlaneConstraints, Object is invalid"); return NULL; }
    OSG::Vec3f v = parseVec3f(args);
    self->obj->setTConstraint(v);
    self->obj->setTConstraintMode(true);
    self->obj->toggleTConstraint(true);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setAxisConstraints(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setAxisConstraints, Object is invalid"); return NULL; }
    OSG::Vec3f v = parseVec3f(args);

    OSG::VRTransform* e = (OSG::VRTransform*) self->obj;
    e->setTConstraint(v);
    e->setTConstraintMode(false);
    e->toggleTConstraint(true);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setRotationConstraints(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setRotationConstraints, Object is invalid"); return NULL; }
    OSG::Vec3f v = parseVec3f(args);
    OSG::Vec3i vi;
    for (int i=0; i<3; i++) vi[i] = v[i];

    OSG::VRTransform* e = (OSG::VRTransform*) self->obj;
    e->setRConstraint(vi);
    e->toggleRConstraint(true);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::duplicate(VRPyTransform* self) {// TODO: can a duplicate for each object be avoided?
    if (self->obj == 0) { PyErr_SetString(err, "C Child is invalid"); return NULL; }
    OSG::VRTransform* d = (OSG::VRTransform*)self->obj->duplicate();
    d->addAttachment("dynamicaly_generated", 0);
    return VRPyTransform::fromPtr( d );
}

PyObject* VRPyTransform::physicalize(VRPyTransform* self, PyObject *args) {
    int b1, b2, b3;
    if (! PyArg_ParseTuple(args, "iii", &b1, &b2, &b3)) return NULL;
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::physicalize: C Object is invalid"); return NULL; }
    OSG::VRTransform* geo = (OSG::VRTransform*) self->obj;

    geo->getPhysics()->setDynamic(b2);
    if (b3) geo->getPhysics()->setShape("Concave");
    else geo->getPhysics()->setShape("Convex");
    geo->getPhysics()->setPhysicalized(b1);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setPhysicsConstraintTo(VRPyTransform* self, PyObject *args) {
    VRPyTransform *t; VRPyConstraint *c; VRPyConstraint *cs;
    if (! PyArg_ParseTuple(args, "OOO", &t, &c, &cs)) return NULL;
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setPhysicsConstraintTo: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->setConstraint( t->obj->getPhysics(), c->obj, cs->obj );
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setMass(VRPyTransform* self, PyObject *args) {
    float f = parseFloat(args);
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setMass: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->setMass(f);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setCollisionMargin(VRPyTransform* self, PyObject *args) {
    float f = parseFloat(args);
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setCollisionMargin: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->setCollisionMargin(f);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setCollisionGroup(VRPyTransform* self, PyObject *args) {
    int i = parseInt(args);
    if (i > 15 or i < 0) { PyErr_SetString(err, "VRPyTransform::setCollisionGroup: only 15 groups/masks available, group 0 means no collisions at all"); return NULL; }
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setCollisionGroup: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->setCollisionGroup(pow(2,i));
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setCollisionMask(VRPyTransform* self, PyObject *args) {
    int i = parseInt(args);
    if (i > 15 or i < 0) { PyErr_SetString(err, "VRPyTransform::setCollisionMask: only 15 groups/masks available, group 0 means no collisions at all"); return NULL; }
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setCollisionMask: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->setCollisionMask(pow(2,i));
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setPhysicsActivationMode(VRPyTransform* self, PyObject *args) {
    int i = parseInt(args);
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setPhysicsActivationMode: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->setActivationMode(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::applyImpulse(VRPyTransform* self, PyObject *args) {
    OSG::Vec3f i = parseVec3f(args);
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::applyImpulse: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->applyImpulse(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::animate(VRPyTransform* self, PyObject *args) {
    VRPyPath* path; float t; float o; int b;
    if (! PyArg_ParseTuple(args, "Offi", &path, &t, &o, &b)) return NULL;
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::animate: C Object is invalid"); return NULL; }
    self->obj->startPathAnimation(path->obj, t, o, b);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::getPhysicsData(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::getPhysicsData: C Object is invalid"); return NULL; }
    btVector3 a = self->obj->getPhysics()->getForce();
    return toPyTuple(OSG::Vec3f(a.getX(),a.getY(),a.getZ()));
}




