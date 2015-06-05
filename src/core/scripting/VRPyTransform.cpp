#include "VRPyTransform.h"
#include "VRPyConstraint.h"
#include "VRPyAnimation.h"
#include "VRPyPath.h"
#include "VRPyBaseT.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRConstraint.h"
#include "core/objects/VRAnimation.h"

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
    {"getWorldDir", (PyCFunction)VRPyTransform::getWorldDir, METH_NOARGS, "Return the object's dir vector" },
    {"getDir", (PyCFunction)VRPyTransform::getDir, METH_NOARGS, "Return the object's dir vector" },
    {"getUp", (PyCFunction)VRPyTransform::getUp, METH_NOARGS, "Return the object's up vector" },
    {"getScale", (PyCFunction)VRPyTransform::getScale, METH_NOARGS, "Return the object's scale vector" },
    {"setWorldFrom", (PyCFunction)VRPyTransform::setWFrom, METH_VARARGS, "Set the object's world position" },
    {"setWorldOrientation", (PyCFunction)VRPyTransform::setWOrientation, METH_VARARGS, "Set the object's world direction" },
    {"setPose", (PyCFunction)VRPyTransform::setPose, METH_VARARGS, "Set the object's from dir && up vector" },
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
    {"physicalize", (PyCFunction)VRPyTransform::physicalize, METH_VARARGS, "physicalize subtree - physicalize( physicalized , dynamic , concave )" },
    {"setGhost", (PyCFunction)VRPyTransform::setGhost, METH_VARARGS, "Set the physics object to be a ghost object - setGhost(bool)" },
    {"setSoft", (PyCFunction)VRPyTransform::setSoft, METH_VARARGS, "Set the physics object to be a soft body - setSoft(bool)" },
    {"attach", (PyCFunction)VRPyTransform::setPhysicsConstraintTo, METH_VARARGS, "create a constraint between this object and another - attach( Transform , Constraint, Spring )" },
    {"detach", (PyCFunction)VRPyTransform::deletePhysicsConstraints, METH_VARARGS, "delete constraint made to this transform with given transform through attach(toTransform). Example call : trans1.detach(trans2)" },
    {"setMass", (PyCFunction)VRPyTransform::setMass, METH_VARARGS, "Set the mass of the physics object" },
    {"setCollisionMargin", (PyCFunction)VRPyTransform::setCollisionMargin, METH_VARARGS, "Set the collision margin of the physics object" },
    {"setCollisionGroup", (PyCFunction)VRPyTransform::setCollisionGroup, METH_VARARGS, "Set the collision group of the physics object" },
    {"setCollisionMask", (PyCFunction)VRPyTransform::setCollisionMask, METH_VARARGS, "Set the collision mask of the physics object" },
    {"setCollisionShape", (PyCFunction)VRPyTransform::setCollisionShape, METH_VARARGS, "Set the collision mask of the physics object" },
    {"getCollisions", (PyCFunction)VRPyTransform::getCollisions, METH_NOARGS, "Return the current collisions with other objects" },
    {"applyImpulse", (PyCFunction)VRPyTransform::applyImpulse, METH_VARARGS, "Apply impulse on the physics object" },
    {"applyForce", (PyCFunction)VRPyTransform::applyForce, METH_VARARGS, "Apply force on the physics object (e.g. obj.applyForce(1.0,0.0,0.0) )" },
    {"applyTorque", (PyCFunction)VRPyTransform::applyTorque, METH_VARARGS, "Apply torque on the physics object  (e.g. obj.applyTorque(1.0,0.0,0.0) )" },
    {"applyConstantForce", (PyCFunction)VRPyTransform::applyConstantForce, METH_VARARGS, "Apply a constant force on the physics object (e.g. obj.applyConstantForce(1.0,0.0,0.0) )" },
    {"applyConstantTorque", (PyCFunction)VRPyTransform::applyConstantTorque, METH_VARARGS, "Apply a constant torque on the physics object  (e.g. obj.applyConstantTorque(1.0,0.0,0.0) )" },
    {"getForce", (PyCFunction)VRPyTransform::getForce, METH_NOARGS, "get the total force put on this transform during this frame. returns 3-Tuple" },
    {"getTorque", (PyCFunction)VRPyTransform::getTorque, METH_NOARGS, "get the total torque put on this transform during this frame. returns 3-Tuple" },
    {"setPhysicsActivationMode", (PyCFunction)VRPyTransform::setPhysicsActivationMode, METH_VARARGS, "Set the physics activation mode of the physics object (normal:1 , no deactivation:4, stay deactivated: 5)" },
    {"animate", (PyCFunction)VRPyTransform::animate, METH_VARARGS, "Animate object (currently only with a path: animate(path, duration, redirect) )" },
    {"getAnimations", (PyCFunction)VRPyTransform::getAnimations, METH_NOARGS, "Return all animations associated to the object" },
    {"animationStop", (PyCFunction)VRPyTransform::animationStop, METH_NOARGS, "Stop any running animation of this object" },
    {"setGravity", (PyCFunction)VRPyTransform::setGravity, METH_VARARGS, "set Gravity (Vector) of given physicalized object" },
    {"getConstraintAngleWith", (PyCFunction)VRPyTransform::getConstraintAngleWith, METH_VARARGS, "return the relative rotation Angles/position diffs (Vector3) to the given constraint partner (if there is one, otherwise return (0.0,0.0,0.0)) example: transform.getConstraintAngleWith(othertransform, 0) returns rotationAngles  (0:rotation , 1:position)"  },
    {"setDamping", (PyCFunction)VRPyTransform::setDamping, METH_VARARGS, "sets the damping of this object. 1st param is the linear, 2nd the angular damping. e.g. physicalizedObject.setDamping(0.4,0.5)"  },
    {"applyChange", (PyCFunction)VRPyTransform::applyChange, METH_VARARGS, "Apply all changes"  },
    {NULL}  /* Sentinel */
};

PyObject* VRPyTransform::applyChange(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::applyChange, Object is invalid"); return NULL; }
    self->obj->update();
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::getCollisions(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setGhost, Object is invalid"); return NULL; }
    auto cols = self->obj->getPhysics()->getCollisions();
    PyObject* res = PyList_New(cols.size());
    int i=0;
    for (auto c : cols) {
        PyObject* cres = PyTuple_New(3);
        PyTuple_SetItem(cres, 0, toPyTuple(c.pos1));
        PyTuple_SetItem(cres, 1, toPyTuple(c.pos2));
        PyTuple_SetItem(cres, 2, toPyTuple(c.norm));
        PyList_SetItem(res, i, cres);
        i++;
    }
    return res;
}

PyObject* VRPyTransform::setGhost(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setGhost, Object is invalid"); return NULL; }
    self->obj->getPhysics()->setGhost(parseBool(args));
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setSoft(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setSoft, Object is invalid"); return NULL; }
    self->obj->getPhysics()->setSoft(parseBool(args));
    Py_RETURN_TRUE;
}
PyObject* VRPyTransform::setDamping(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setDamping, Object is invalid"); return NULL; }
    float lin,ang;
    if (! PyArg_ParseTuple(args, "ff", &lin, &ang)) return NULL;
    self->obj->getPhysics()->setDamping(lin,ang);
    Py_RETURN_TRUE;
}

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

PyObject* VRPyTransform::getWorldDir(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::getWorldDir, Object is invalid"); return NULL; }
    return toPyTuple(self->obj->getWorldDirection());
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

PyObject* VRPyTransform::setWOrientation(VRPyTransform* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setWDir, Object is invalid"); return NULL; }
    PyObject *d, *u;
    if (! PyArg_ParseTuple(args, "OO", &d, &u)) return NULL;
    self->obj->setWorldOrientation(parseVec3fList(d), parseVec3fList(u));
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
    if (i > 15 || i < 0) { PyErr_SetString(err, "VRPyTransform::setCollisionGroup: only 15 groups/masks available, group 0 means no collisions at all"); return NULL; }
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setCollisionGroup: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->setCollisionGroup(pow(2,i));
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setCollisionShape(VRPyTransform* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setCollisionShape C Object is invalid"); return NULL; }
    PyObject* shape; float param;
    if (! PyArg_ParseTuple(args, "Of", &shape, &param)) return NULL;
    self->obj->getPhysics()->setShape( PyString_AsString(shape), param );
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setCollisionMask(VRPyTransform* self, PyObject *args) {
    int i = parseInt(args);
    if (i > 15 || i < 0) { PyErr_SetString(err, "VRPyTransform::setCollisionMask: only 15 groups/masks available, group 0 means no collisions at all"); return NULL; }
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

PyObject* VRPyTransform::applyForce(VRPyTransform* self, PyObject *args) {
    OSG::Vec3f i = parseVec3f(args);
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::applyForce: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->addForce(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::applyConstantForce(VRPyTransform* self, PyObject *args) {
    OSG::Vec3f i = parseVec3f(args);
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::applyForce: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->addConstantForce(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::applyTorque(VRPyTransform* self, PyObject *args) {
    OSG::Vec3f i = parseVec3f(args);
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::applyTorque: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->addTorque(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::applyConstantTorque(VRPyTransform* self, PyObject *args) {
    OSG::Vec3f i = parseVec3f(args);
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::applyTorque: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->addConstantTorque(i);
    Py_RETURN_TRUE;
}


PyObject* VRPyTransform::getForce(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::getForce: C Object is invalid"); return NULL; }
    OSG::Vec3f i = self->obj->getPhysics()->getTorque();
     return toPyTuple(i);
}

PyObject* VRPyTransform::getTorque(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::getTorque: C Object is invalid"); return NULL; }
    OSG::Vec3f i = self->obj->getPhysics()->getForce();
    return toPyTuple(i);
}

PyObject* VRPyTransform::setGravity(VRPyTransform* self, PyObject *args) {
    OSG::Vec3f i = parseVec3f(args);
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::setGravity: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->setGravity(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::animate(VRPyTransform* self, PyObject *args) {
    VRPyPath* path = 0; float t; float o; int b;
    int l = 0;
    if (pySize(args) == 4)
        if (! PyArg_ParseTuple(args, "Offi", &path, &t, &o, &b)) return NULL;
    if (pySize(args) == 5)
        if (! PyArg_ParseTuple(args, "Offii", &path, &t, &o, &b, &l)) return NULL;
	if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::animate: C Object is invalid"); return NULL; }
	if (path == 0) { PyErr_SetString(err, "VRPyTransform::animate: path is invalid"); return NULL; }
    self->obj->startPathAnimation(path->obj, t, o, b, l);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::getAnimations(VRPyTransform* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::getAnimations: C Object is invalid"); return NULL; }
    vector<OSG::VRAnimation*> anims = self->obj->getAnimations();

    PyObject* li = PyList_New(anims.size());
    for (uint i=0; i<anims.size(); i++) {
        PyList_SetItem(li, i, VRPyAnimation::fromPtr(anims[i]));
    }
    return li;
}

PyObject* VRPyTransform::getConstraintAngleWith(VRPyTransform* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::getConstraintAngleWith: C Object is invalid"); return NULL; }
    VRPyTransform *t;
    int rotationOrPosition = 0;
    if (! PyArg_ParseTuple(args, "Oi",&t, &rotationOrPosition)) return NULL;
    OSG::Vec3f a = OSG::Vec3f(0.0,0.0,0.0);
    //cout << (self->obj->getPhysics()->getConstraintAngle(t->obj->getPhysics(),rotationOrPosition));
    if(rotationOrPosition == 0) {
        a[0] = (self->obj->getPhysics()->getConstraintAngle(t->obj->getPhysics(),0));
        a[1] = (self->obj->getPhysics()->getConstraintAngle(t->obj->getPhysics(),1));
        a[2] = (self->obj->getPhysics()->getConstraintAngle(t->obj->getPhysics(),2));
    }
    else if(rotationOrPosition == 1) {
        a[0] = (self->obj->getPhysics()->getConstraintAngle(t->obj->getPhysics(),3));
        a[1] = (self->obj->getPhysics()->getConstraintAngle(t->obj->getPhysics(),4));
        a[2] = (self->obj->getPhysics()->getConstraintAngle(t->obj->getPhysics(),5));
    }

    //Py_RETURN_TRUE;
    return toPyTuple(a);
}

PyObject* VRPyTransform::deletePhysicsConstraints(VRPyTransform* self, PyObject *args) {
    VRPyTransform *t;
    if (! PyArg_ParseTuple(args, "O", &t)) return NULL;
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTransform::deletePhysicsConstraints: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->deleteConstraints(t->obj->getPhysics());
    Py_RETURN_TRUE;
}
