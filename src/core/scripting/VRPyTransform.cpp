#include "VRPyTransform.h"
#include "VRPyConstraint.h"
#include "VRPyAnimation.h"
#include "VRPyPose.h"
#include "VRPyPath.h"
#include "VRPyIntersection.h"
#include "VRPyTypeCaster.h"
#include "VRPyMath.h"
#include "VRPyBaseT.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRConstraint.h"
#include "core/objects/VRAnimation.h"
#include "core/setup/devices/VRIntersect.h"

using namespace OSG;

simpleVRPyType(Collision, 0);
simpleVRPyType(Transform, New_VRObjects_ptr);

template<> PyObject* VRPyTypeCaster::cast(const VRCollision& e) { return VRPyCollision::fromObject(e); }

PyMethodDef VRPyCollision::methods[] = {
    {"getPos1", PyWrap(Collision, getPos1, "Get the first collision point", Vec3d)  },
    {"getPos2", PyWrap(Collision, getPos2, "Get the second collision point", Vec3d)  },
    {"getNorm", PyWrap(Collision, getNorm, "Get collision normal vector", Vec3d)  },
    {"getDistance", PyWrap(Collision, getDistance, "Get the distance between collision points", float)  },
    {"getObj1", PyWrap(Collision, getObj1, "Get the first collision object", VRTransformPtr)  },
    {"getObj2", PyWrap(Collision, getObj2, "Get the second collision object", VRTransformPtr)  },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyTransform::methods[] = {
    {"setIdentity", (PyCFunction)VRPyTransform::setIdentity, METH_NOARGS, "Reset transformation to identity" },
    {"translate", (PyCFunction)VRPyTransform::translate, METH_VARARGS, "Translate the object along a vector - translate(xf,yf,zf)" },
    {"move", (PyCFunction)VRPyTransform::move, METH_VARARGS, "Move the object - move(d)" },
    {"rotate", (PyCFunction)VRPyTransform::rotate, METH_VARARGS, "Rotate the object around an axis - rotate(xf,yf,zf,af)" },
    {"rotateAround", (PyCFunction)VRPyTransform::rotateAround, METH_VARARGS, "Rotate the object around its at - rotateAround(af)" },
    {"rotateYonZ", PyWrap(Transform, rotateYonZ, "Rotate the Y axis onto the Z axis", void) },
    {"getFrom", (PyCFunction)VRPyTransform::getFrom, METH_NOARGS, "Return the object's from vector" },
    {"getAt", (PyCFunction)VRPyTransform::getAt, METH_NOARGS, "Return the object's at vector" },
    {"getDir", (PyCFunction)VRPyTransform::getDir, METH_NOARGS, "Return the object's dir vector" },
    {"getUp", (PyCFunction)VRPyTransform::getUp, METH_NOARGS, "Return the object's up vector" },
    {"getPose", (PyCFunction)VRPyTransform::getPose, METH_NOARGS, "Return the object's pose - pose getPose()" },
    {"getWorldPose", (PyCFunction)VRPyTransform::getWorldPose, METH_NOARGS, "Return the object's world pose - pose getWorldPose()" },
    {"getWorldFrom", (PyCFunction)VRPyTransform::getWFrom, METH_NOARGS, "Return the object's world position" },
    {"getWorldDir", (PyCFunction)VRPyTransform::getWorldDir, METH_NOARGS, "Return the object's dir vector" },
    {"getWorldUp", (PyCFunction)VRPyTransform::getWorldUp, METH_NOARGS, "Return the object's up vector" },
    {"getWorldAt", PyWrapOpt(Transform, getWorldAt, "Return the object's at vector", "0", Vec3d, bool ) },
    {"getScale", (PyCFunction)VRPyTransform::getScale, METH_NOARGS, "Return the object's scale vector" },
    {"getEuler", (PyCFunction)VRPyTransform::getEuler, METH_NOARGS, "Return the object's euler angles - [X,Y,Z] getEuler()" },
    {"setMatrix", PyWrap(Transform, setMatrix, "Set the object's matrix", void, Matrix4d ) },
    {"setWorldMatrix", PyWrap(Transform, setWorldMatrix, "Set the object's world matrix", void, Matrix4d ) },
    {"setWorldFrom", (PyCFunction)VRPyTransform::setWFrom, METH_VARARGS, "Set the object's world position" },
    {"setWorldOrientation", (PyCFunction)VRPyTransform::setWOrientation, METH_VARARGS, "Set the object's world direction" },
    {"setWorldUp", PyWrap(Transform, setWorldUp, "Set the object's up vector", void, Vec3d ) },
    {"setWorldAt", PyWrap(Transform, setWorldAt, "Set the object's at vector", void, Vec3d ) },
    {"setPose", (PyCFunction)VRPyTransform::setPose, METH_VARARGS, "Set the object's pose - setPose(pose)\n\tsetPose(pos, dir, up)" },
    {"setWorldPose", (PyCFunction)VRPyTransform::setWPose, METH_VARARGS, "Set the object's pose - setWorldPose(pose)\n\tsetPose(pos, dir, up)" },
    {"setPosition", (PyCFunction)VRPyTransform::setFrom, METH_VARARGS, "Set the object's from vector" },
    {"setFrom", (PyCFunction)VRPyTransform::setFrom, METH_VARARGS, "Set the object's from vector" },
    {"setAt", (PyCFunction)VRPyTransform::setAt, METH_VARARGS, "Set the object's at vector" },
    {"setDir", (PyCFunction)VRPyTransform::setDir, METH_VARARGS, "Set the object's dir vector" },
    {"setEuler", (PyCFunction)VRPyTransform::setEuler, METH_VARARGS, "Set the object's orientation using Euler angles - setEuler(x,y,z)" },
    {"setUp", (PyCFunction)VRPyTransform::setUp, METH_VARARGS, "Set the object's up vector" },
    {"setScale", (PyCFunction)VRPyTransform::setScale, METH_VARARGS, "Set the object's scale vector" },
    {"setPointConstraints", (PyCFunction)VRPyTransform::setPointConstraints, METH_VARARGS, "Constraint the object on a point - setPointConstraints(x, y, z)" },
    {"setPlaneConstraints", (PyCFunction)VRPyTransform::setPlaneConstraints, METH_VARARGS, "Constraint the object on a plane - setPlaneConstraints(nxf, nyf, nzf)" },
    {"setAxisConstraints", (PyCFunction)VRPyTransform::setAxisConstraints, METH_VARARGS, "Constraint the object on an axis - TODO -> to test, may work" },
    {"setRotationConstraints", (PyCFunction)VRPyTransform::setRotationConstraints, METH_VARARGS, "Constraint the object's rotation - setRotationConstraints(xi, yi, zi)" },
    {"setConstraint", (PyCFunction)VRPyTransform::setConstraint, METH_VARARGS, "Set the constraints object - setConstraint( constraint )" },
    {"getConstraint", (PyCFunction)VRPyTransform::getConstraint, METH_NOARGS, "Get the constraints object - constraint getConstraint()" },
    {"physicalize", (PyCFunction)VRPyTransform::physicalize, METH_VARARGS, "physicalize subtree - physicalize( bool physicalized , bool dynamic , str shape, float shape param )\n\tshape can be: ['Box', 'Sphere', 'Convex', 'Concave', 'ConvexDecomposed']" },
    {"setGhost", (PyCFunction)VRPyTransform::setGhost, METH_VARARGS, "Set the physics object to be a ghost object - setGhost(bool)" },
    {"attach", (PyCFunction)VRPyTransform::setPhysicsConstraintTo, METH_VARARGS,
        "create a constraint between this object and another - \n"
        " For rigid bodies: attach( Transform , VRConstraint constraint, VRConstraint spring )\n"
        " For soft bodies: attach( Transform, int nodeIndex, local pivot [x,y,z], bool ignoreCollision, float influence" },
    {"detach", (PyCFunction)VRPyTransform::deletePhysicsConstraints, METH_VARARGS, "delete constraint made to this transform with given transform through attach(toTransform). Example call : trans1.detach(trans2)" },
    {"setMass", (PyCFunction)VRPyTransform::setMass, METH_VARARGS, "Set the mass of the physics object" },
    {"setCollisionMargin", (PyCFunction)VRPyTransform::setCollisionMargin, METH_VARARGS, "Set the collision margin of the physics object" },
    {"setCollisionGroup", (PyCFunction)VRPyTransform::setCollisionGroup, METH_VARARGS, "Set the collision group of the physics object - setCollisionGroup(int g)\n\t g can be from 0 to 8" },
    {"setCollisionMask", (PyCFunction)VRPyTransform::setCollisionMask, METH_VARARGS, "Set the collision mask of the physics object - setCollisionMask(int g)\n\t g can be from 0 to 8 and it is the group to collide with" },
    {"setCollisionShape", (PyCFunction)VRPyTransform::setCollisionShape, METH_VARARGS, "Set the collision shape of the physics object, see physicalize - setCollisionShape( str shape, float param )" },
    {"getCollisions", PyWrap(Transform, getCollisions, "Return the current collisions with other objects", vector<VRCollision> ) },
    {"applyImpulse", (PyCFunction)VRPyTransform::applyImpulse, METH_VARARGS, "Apply impulse on the physics object" },
    {"applyTorqueImpulse", (PyCFunction)VRPyTransform::applyTorqueImpulse, METH_VARARGS, "Apply torque impulse on the physics object" },
    {"applyForce", (PyCFunction)VRPyTransform::applyForce, METH_VARARGS, "Apply force on the physics object (e.g. obj.applyForce(1.0,0.0,0.0) )" },
    {"applyTorque", (PyCFunction)VRPyTransform::applyTorque, METH_VARARGS, "Apply torque on the physics object  (e.g. obj.applyTorque(1.0,0.0,0.0) )" },
    {"applyConstantForce", (PyCFunction)VRPyTransform::applyConstantForce, METH_VARARGS, "Apply a constant force on the physics object (e.g. obj.applyConstantForce(1.0,0.0,0.0) )" },
    {"applyConstantTorque", (PyCFunction)VRPyTransform::applyConstantTorque, METH_VARARGS, "Apply a constant torque on the physics object  (e.g. obj.applyConstantTorque(1.0,0.0,0.0) )" },
    {"getForce", (PyCFunction)VRPyTransform::getForce, METH_NOARGS, "get the total force put on this transform during this frame. returns 3-Tuple" },
    {"getTorque", (PyCFunction)VRPyTransform::getTorque, METH_NOARGS, "get the total torque put on this transform during this frame. returns 3-Tuple" },
    {"setPhysicsActivationMode", (PyCFunction)VRPyTransform::setPhysicsActivationMode, METH_VARARGS, "Set the physics activation mode of the physics object (normal:1 , no deactivation:4, stay deactivated: 5)" },
    {"setPhysicalizeTree", (PyCFunction)VRPyTransform::setPhysicalizeTree, METH_VARARGS, "Set to physicalize whole tree or just current node - setPhysicalizeTree( bool b )" },
    {"animate", (PyCFunction)VRPyTransform::animate, METH_VARARGS, "Animate object along a path:\n "
                                                                    "animate(path, float duration [s], float offset [s], bool redirect) )\n"
                                                                    "animate(path, float duration [s], float offset [s], bool redirect, bool loop) )" },
    {"getAnimations", (PyCFunction)VRPyTransform::getAnimations, METH_NOARGS, "Return all animations associated to the object" },
    {"animationStop", (PyCFunction)VRPyTransform::animationStop, METH_NOARGS, "Stop any running animation of this object" },
    {"setGravity", (PyCFunction)VRPyTransform::setGravity, METH_VARARGS, "set Gravity (Vector) of given physicalized object" },
    {"getConstraintAngleWith", (PyCFunction)VRPyTransform::getConstraintAngleWith, METH_VARARGS, "return the relative rotation Angles/position diffs (Vector3) to the given constraint partner (if there is one, otherwise return (0.0,0.0,0.0)) example: transform.getConstraintAngleWith(othertransform, 0) returns rotationAngles  (0:rotation , 1:position)"  },
    {"setDamping", (PyCFunction)VRPyTransform::setDamping, METH_VARARGS, "sets the damping of this object. 1st param is the linear, 2nd the angular damping. e.g. physicalizedObject.setDamping(0.4,0.5)"  },
    {"applyChange", (PyCFunction)VRPyTransform::applyChange, METH_NOARGS, "Apply all changes"  },
    {"setCenterOfMass", (PyCFunction)VRPyTransform::setCenterOfMass, METH_VARARGS, "Set a custom center of mass - setCenterOfMass([x,y,z])"  },
    {"drag", (PyCFunction)VRPyTransform::drag, METH_VARARGS, "Drag this object by new parent - drag(new parent)"  },
    {"drop", (PyCFunction)VRPyTransform::drop, METH_NOARGS, "Drop this object, if held, to old parent - drop()"  },
    {"rebaseDrag", PyWrap( Transform, rebaseDrag, "Rebase drag, use instead of switchParent", void, VRObjectPtr ) },
    {"isDragged", PyWrap( Transform, isDragged, "Check if transform is beeing dragged", bool ) },
    {"castRay", (PyCFunction)VRPyTransform::castRay, METH_VARARGS, "Cast a ray and return the intersection - intersection castRay(obj, dir)"  },
    {"getDragParent", (PyCFunction)VRPyTransform::getDragParent, METH_NOARGS, "Get the parent before the drag started - obj getDragParent()"  },
    {"lastChanged", (PyCFunction)VRPyTransform::lastChanged, METH_NOARGS, "Return the frame when the last change occured - lastChanged()"  },

    {"setWorldDir", PyWrap(Transform, setWorldDir, "Set the direction in world coordinates", void, Vec3d)  },
    {"setWorldUp", PyWrap(Transform, setWorldUp, "Set the up vector in world coordinates", void, Vec3d)  },
    {"getPoseTo", PyWrap(Transform, getPoseTo, "Get the pose in the coordinate system of another object", PosePtr, VRObjectPtr)  },
    {"applyTransformation", (PyCFunction)VRPyTransform::applyTransformation, METH_VARARGS, "Apply a transformation to the mesh - applyTransformation( pose )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyTransform::fromSharedPtr(VRTransformPtr obj) {
    return VRPyTypeCaster::cast(dynamic_pointer_cast<VRObject>(obj));
}

PyObject* VRPyTransform::applyTransformation(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    VRPyPose* pose = 0;
    if (!PyArg_ParseTuple(args, "|O", &pose)) return NULL;
    if (pose) self->objPtr->applyTransformation( pose->objPtr );
    else self->objPtr->applyTransformation();
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setConstraint(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyConstraint* c = 0;
    if (! PyArg_ParseTuple(args, "O", &c)) return NULL;
    self->objPtr->setConstraint(c->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::getConstraint(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    return VRPyConstraint::fromSharedPtr( self->objPtr->getConstraint() );
}

PyObject* VRPyTransform::lastChanged(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    return PyInt_FromLong( self->objPtr->getLastChange() );
}

PyObject* VRPyTransform::setPhysicalizeTree(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->getPhysics()->physicalizeTree( parseBool(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::castRay(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyObject* o = 0; PyObject* d;
    if (! PyArg_ParseTuple(args, "OO", &o, &d)) return NULL;
    auto line = self->objPtr->castRay( 0, parseVec3dList(d) );
    OSG::VRIntersect in;
    return VRPyIntersection::fromObject( in.intersect(o->objPtr, line) );
}

PyObject* VRPyTransform::getDragParent(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    auto p = self->objPtr->getDragParent();
    return VRPyTypeCaster::cast( p );
}

PyObject* VRPyTransform::drag(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyTransform* t = 0; parseObject(args, t);
    self->objPtr->drag( t->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::drop(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    self->objPtr->drop();
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setEuler(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->setEuler(parseVec3d(args));
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setCenterOfMass(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->getPhysics()->setCenterOfMass(parseVec3d(args));
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::applyChange(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    self->objPtr->updateChange();
    Py_RETURN_TRUE;
}

/*PyObject* VRPyTransform::getCollisions(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    auto cols = self->objPtr->getPhysics()->getCollisions();
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
}*/

PyObject* VRPyTransform::setGhost(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->getPhysics()->setGhost(parseBool(args));
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setDamping(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    float lin,ang;
    if (! PyArg_ParseTuple(args, "ff", &lin, &ang)) return NULL;
    self->objPtr->getPhysics()->setDamping(lin,ang);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::animationStop(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    self->objPtr->stopAnimation();
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setIdentity(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    self->objPtr->setMatrix(OSG::Matrix4d());
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::translate(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->translate( parseVec3d(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::move(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    float t = parseFloat(args);
    self->objPtr->move(t);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::rotate(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec4d r = parseVec4d(args);

    OSG::Vec3d axis = OSG::Vec3d(r);
    float angle = r[3];

    OSG::VRTransformPtr e = (OSG::VRTransformPtr) self->objPtr;
    e->rotate(angle, axis);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::rotateAround(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->rotateAround( parseFloat(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::getEuler(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    return toPyObject(self->objPtr->getEuler());
}

PyObject* VRPyTransform::getFrom(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    return toPyObject(self->objPtr->getFrom());
}

PyObject* VRPyTransform::getWFrom(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    return toPyObject(self->objPtr->getWorldPosition());
}

PyObject* VRPyTransform::getAt(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    return toPyObject(self->objPtr->getAt());
}

PyObject* VRPyTransform::getDir(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    return toPyObject(self->objPtr->getDir());
}

PyObject* VRPyTransform::getWorldDir(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    return toPyObject(self->objPtr->getWorldDirection());
}

PyObject* VRPyTransform::getWorldUp(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    return toPyObject(self->objPtr->getWorldUp());
}

PyObject* VRPyTransform::getUp(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    return toPyObject(self->objPtr->getUp());
}

PyObject* VRPyTransform::getScale(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    return toPyObject(self->objPtr->getScale());
}

PyObject* VRPyTransform::getPose(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    return VRPyPose::fromSharedPtr( self->objPtr->getPose() );
}

PyObject* VRPyTransform::getWorldPose(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    return VRPyPose::fromSharedPtr( self->objPtr->getWorldPose() );
}

PyObject* VRPyTransform::setPose(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    if (pySize(args) == 1) {
        VRPyPose* p;
        if (! PyArg_ParseTuple(args, "O", &p)) return NULL;
        if (p->objPtr) self->objPtr->setPose( p->objPtr );
        Py_RETURN_TRUE;
    }

    PyObject *fl, *dl, *ul;
    if (! PyArg_ParseTuple(args, "OOO", &fl, &dl, &ul)) return NULL;
    self->objPtr->setPose( parseVec3dList(fl), parseVec3dList(dl), parseVec3dList(ul));
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setWPose(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    if (pySize(args) == 1) {
        VRPyPose* p;
        if (! PyArg_ParseTuple(args, "O", &p)) return NULL;
        if (p->objPtr) self->objPtr->setWorldPose( p->objPtr );
        Py_RETURN_TRUE;
    }

    PyObject *fl, *dl, *ul;
    if (! PyArg_ParseTuple(args, "OOO", &fl, &dl, &ul)) return NULL;
    self->objPtr->setWorldPose( Pose::create(parseVec3dList(fl), parseVec3dList(dl), parseVec3dList(ul)) );
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setFrom(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d v = parseVec3d(args);
    self->objPtr->setFrom(v);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setWFrom(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d v = parseVec3d(args);
    self->objPtr->setWorldPosition(v);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setWOrientation(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject *d, *u;
    if (! PyArg_ParseTuple(args, "OO", &d, &u)) return NULL;
    self->objPtr->setWorldOrientation(parseVec3dList(d), parseVec3dList(u));
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setAt(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d v = parseVec3d(args);
    self->objPtr->setAt(v);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setDir(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d v = parseVec3d(args);
    self->objPtr->setDir(v);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setUp(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d v = parseVec3d(args);
    self->objPtr->setUp(v);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setScale(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d v = parseVec3d(args);
    self->objPtr->setScale(v);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setPointConstraints(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d v = parseVec3d(args);
    auto c = self->objPtr->getConstraint();
    c->setTConstraint(v, OSG::VRConstraint::POINT);
    c->setActive(true, self->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setPlaneConstraints(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d v = parseVec3d(args);
    auto c = self->objPtr->getConstraint();
    c->setTConstraint(v, OSG::VRConstraint::PLANE);
    c->setActive(true, self->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setAxisConstraints(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d v = parseVec3d(args);
    auto c = self->objPtr->getConstraint();
    c->setTConstraint(v, OSG::VRConstraint::LINE);
    c->setActive(true, self->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setRotationConstraints(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d v = parseVec3d(args);
    auto c = self->objPtr->getConstraint();
    c->setRConstraint(self->objPtr->getWorldPosition(), OSG::VRConstraint::POINT);
    c->setActive(true, self->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::physicalize(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    int doPhys = 1;
    int isDyn = 1;
    int deprc_shp = 1;
    const char* shape = "Convex";
    float param = 0;

    if ( pySize(args) == 3 && PyInt_Check(PyTuple_GetItem(args, 2)) ) {
        if ( !PyArg_ParseTuple(args, "iii", &doPhys, &isDyn, &deprc_shp)) return NULL; // backwards compatibility
        else shape = deprc_shp == 1 ? "Concave" : "Convex";
    } else if (! PyArg_ParseTuple(args, "|iisf", &doPhys, &isDyn, (char*)&shape, &param)) return NULL;
    OSG::VRTransformPtr geo = (OSG::VRTransformPtr) self->objPtr;

    if (auto p = geo->getPhysics()) {
        p->setDynamic(isDyn);
        p->setShape(shape, param);
        p->setPhysicalized(doPhys);
    }

    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setPhysicsConstraintTo(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    //if this is soft, the args have to be: RigidBody other, int nodeIndex, vec3 localpivot, bool ignoreCollision, float influence
    VRPyTransform *t;
    if(self->objPtr->getPhysics()->isSoft()) {
        int nodeIndex;
        int ignoreCollision;
        float influence;
        PyObject* localPiv;
        if (! PyArg_ParseTuple(args, "OiOif", &t, &nodeIndex, &localPiv, &ignoreCollision, &influence)) return NULL;
        self->objPtr->getPhysics()->setConstraint(t->objPtr->getPhysics(), nodeIndex, parseVec3dList(localPiv), ignoreCollision, influence);
    }
    else {
        VRPyTransform *t; VRPyConstraint *c; VRPyConstraint *cs;
        if (! PyArg_ParseTuple(args, "OOO", &t, &c, &cs)) return NULL;
        self->objPtr->getPhysics()->setConstraint( t->objPtr->getPhysics(), c->objPtr, cs->objPtr );
        self->objPtr->setConstraint(c->objPtr);
    }
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setMass(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    float f = parseFloat(args);
    self->objPtr->getPhysics()->setMass(f);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setCollisionMargin(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    float f = parseFloat(args);
    self->objPtr->getPhysics()->setCollisionMargin(f);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setCollisionGroup(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    int m = 0;
    PyObject* p = 0;
    if (! PyArg_ParseTuple(args, "O", &p)) return NULL;
    if (!isList(p)) m = pow(2, PyInt_AsLong(p) );
    else for(int i=0; i<pySize(p); i++) {
        PyObject* pi = PyList_GetItem(p, i);
        m = m | int( pow(2,PyInt_AsLong(pi)) );
    }
    self->objPtr->getPhysics()->setCollisionGroup( m );
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setCollisionShape(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    PyObject* shape; float param;
    if (! PyArg_ParseTuple(args, "Of", &shape, &param)) return NULL;
    self->objPtr->getPhysics()->setShape( PyString_AsString(shape), param );
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setCollisionMask(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    int m = 0;
    PyObject* p = 0;
    if (! PyArg_ParseTuple(args, "O", &p)) return NULL;
    if (!isList(p)) m = pow(2, PyInt_AsLong(p) );
    else for(int i=0; i<pySize(p); i++) {
        PyObject* pi = PyList_GetItem(p, i);
        m = m | int( pow(2,PyInt_AsLong(pi)) );
    }
    self->objPtr->getPhysics()->setCollisionMask( m );
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setPhysicsActivationMode(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    int i = parseInt(args);
    self->objPtr->getPhysics()->setActivationMode(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::applyImpulse(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d i = parseVec3d(args);
    self->objPtr->getPhysics()->applyImpulse(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::applyTorqueImpulse(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d i = parseVec3d(args);
    self->objPtr->getPhysics()->applyTorqueImpulse(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::applyForce(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d i = parseVec3d(args);
    self->objPtr->getPhysics()->addForce(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::applyConstantForce(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d i = parseVec3d(args);
    self->objPtr->getPhysics()->addConstantForce(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::applyTorque(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d i = parseVec3d(args);
    self->objPtr->getPhysics()->addTorque(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::applyConstantTorque(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d i = parseVec3d(args);
    self->objPtr->getPhysics()->addConstantTorque(i);
    Py_RETURN_TRUE;
}


PyObject* VRPyTransform::getForce(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    OSG::Vec3d i = self->objPtr->getPhysics()->getForce();
    return toPyObject(i);
}

PyObject* VRPyTransform::getTorque(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    OSG::Vec3d i = self->objPtr->getPhysics()->getTorque();
    return toPyObject(i);
}

PyObject* VRPyTransform::setGravity(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d i = parseVec3d(args);
    self->objPtr->getPhysics()->setGravity(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::animate(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    VRPyPath* path = 0; float t; float o; int b;
    int l = 0;
    if (! PyArg_ParseTuple(args, "Offi|i", &path, &t, &o, &b, &l)) return NULL;
	if (path == 0) { PyErr_SetString(err, "VRPyTransform::animate: path is invalid"); return NULL; }
    auto anim = self->objPtr->startPathAnimation(path->objPtr, t, o, b, l);
    return VRPyAnimation::fromSharedPtr(anim);
}

PyObject* VRPyTransform::getAnimations(VRPyTransform* self) {
    if (!self->valid()) return NULL;
    auto anims = self->objPtr->getAnimations();
    PyObject* li = PyList_New(anims.size());
    for (uint i=0; i<anims.size(); i++) {
        PyList_SetItem(li, i, VRPyAnimation::fromSharedPtr(anims[i]));
    }
    return li;
}

PyObject* VRPyTransform::getConstraintAngleWith(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    VRPyTransform *t;
    int rotationOrPosition = 0;
    if (! PyArg_ParseTuple(args, "Oi",&t, &rotationOrPosition)) return NULL;
    OSG::Vec3d a = OSG::Vec3d(0.0,0.0,0.0);
    //cout << (self->objPtr->getPhysics()->getConstraintAngle(t->obj->getPhysics(),rotationOrPosition));
    if(rotationOrPosition == 0) {
        a[0] = (self->objPtr->getPhysics()->getConstraintAngle(t->objPtr->getPhysics(),0));
        a[1] = (self->objPtr->getPhysics()->getConstraintAngle(t->objPtr->getPhysics(),1));
        a[2] = (self->objPtr->getPhysics()->getConstraintAngle(t->objPtr->getPhysics(),2));
    }
    else if(rotationOrPosition == 1) {
        a[0] = (self->objPtr->getPhysics()->getConstraintAngle(t->objPtr->getPhysics(),3));
        a[1] = (self->objPtr->getPhysics()->getConstraintAngle(t->objPtr->getPhysics(),4));
        a[2] = (self->objPtr->getPhysics()->getConstraintAngle(t->objPtr->getPhysics(),5));
    }

    //Py_RETURN_TRUE;
    return toPyObject(a);
}

PyObject* VRPyTransform::deletePhysicsConstraints(VRPyTransform* self, PyObject *args) {
    if (!self->valid()) return NULL;
    VRPyTransform *t;
    if (! PyArg_ParseTuple(args, "O", &t)) return NULL;
    self->objPtr->getPhysics()->deleteConstraints(t->objPtr->getPhysics());
    Py_RETURN_TRUE;
}
