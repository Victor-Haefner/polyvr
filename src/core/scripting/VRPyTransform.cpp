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
    {"getPos1", PyWrap(Collision, getPos1, "Get the first collision point", Vec3d) },
    {"getPos2", PyWrap(Collision, getPos2, "Get the second collision point", Vec3d) },
    {"getNorm", PyWrap(Collision, getNorm, "Get collision normal vector", Vec3d) },
    {"getDistance", PyWrap(Collision, getDistance, "Get the distance between collision points", float) },
    {"getObj1", PyWrap(Collision, getObj1, "Get the first collision object", VRTransformPtr) },
    {"getObj2", PyWrap(Collision, getObj2, "Get the second collision object", VRTransformPtr) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyTransform::methods[] = {
    {"setIdentity", PyWrap(Transform, setIdentity, "Reset transformation to identity", void ) },
    {"translate", PyWrap(Transform, translate, "Translate the object along a vector", void, Vec3d ) },
    {"move", PyWrap(Transform, move, "Move the object in dir direction", void, float ) },
    {"rotate", PyWrap(Transform, rotate, "Rotate the object around an axis and angle", void, float, Vec3d ) },
    {"rotateAround", PyWrap(Transform, rotateAround, "Rotate the object around its at axis", void, float ) },
    {"rotateYonZ", PyWrap(Transform, rotateYonZ, "Rotate the Y axis onto the Z axis", void ) },
    {"getFrom", PyWrap(Transform, getFrom, "Return the object's from vector", Vec3d ) },
    {"getAt", PyWrap(Transform, getAt, "Return the object's at vector", Vec3d ) },
    {"getDir", PyWrap(Transform, getDir, "Return the object's dir vector", Vec3d ) },
    {"getUp", PyWrap(Transform, getUp, "Return the object's up vector", Vec3d ) },
    {"getPose", PyWrap(Transform, getPose, "Return the object's pose", PosePtr ) },
    {"getWorldPose", PyWrap(Transform, getWorldPose, "Return the object's world pose", PosePtr ) },
    {"getWorldFrom", PyWrapOpt(Transform, getWorldPosition, "Return the object's world position", "0", Vec3d, bool ) },
    {"getWorldDir", PyWrapOpt(Transform, getWorldDirection, "Return the object's dir vector", "0", Vec3d, bool ) },
    {"getWorldUp", PyWrapOpt(Transform, getWorldUp, "Return the object's up vector", "0", Vec3d, bool ) },
    {"getWorldAt", PyWrapOpt(Transform, getWorldAt, "Return the object's at vector", "0", Vec3d, bool ) },
    {"getScale", PyWrap(Transform, getScale, "Return the object's scale vector", Vec3d ) },
    {"getEuler", PyWrap(Transform, getEuler, "Return the object's euler angles", Vec3d ) },
    {"setMatrix", PyWrap(Transform, setMatrix, "Set the object's matrix", void, Matrix4d ) },
    {"setWorldMatrix", PyWrap(Transform, setWorldMatrix, "Set the object's world matrix", void, Matrix4d ) },
    {"setWorldFrom", PyWrap(Transform, setWorldPosition, "Set the object's world position", void, Vec3d ) },
    {"setWorldOrientation", PyWrap(Transform, setWorldOrientation, "Set the object's world direction", void, Vec3d, Vec3d ) },
    {"setWorldUp", PyWrap(Transform, setWorldUp, "Set the object's up vector", void, Vec3d ) },
    {"setWorldAt", PyWrap(Transform, setWorldAt, "Set the object's at vector", void, Vec3d ) },
    {"setPose", PyWrap(Transform, setPose, "Set the object's pose", void, PosePtr ) },
    {"setTransform", PyWrapOpt(Transform, setTransform, "Set the object's pose", "0 0 -1|0 1 0", void, Vec3d, Vec3d, Vec3d ) },
    {"setWorldPose", PyWrap(Transform, setWorldPose, "Set the object's pose", void, PosePtr ) },
    {"setPosition", PyWrap(Transform, setFrom, "Set the object's from vector", void, Vec3d ) },
    {"setFrom", PyWrap(Transform, setFrom, "Set the object's from vector", void, Vec3d ) },
    {"setAt", PyWrap(Transform, setAt, "Set the object's at vector", void, Vec3d ) },
    {"setDir", PyWrap(Transform, setDir, "Set the object's dir vector", void, Vec3d ) },
    {"setEuler", PyWrap(Transform, setEuler, "Set the object's orientation using Euler angles - setEuler(x,y,z)", void, Vec3d ) },
    {"setUp", PyWrap(Transform, setUp, "Set the object's up vector", void, Vec3d ) },
    {"setScale", PyWrap(Transform, setScale, "Set the object's scale vector", void, Vec3d ) },
    //{"setPointConstraints", PyWrap(Transform, setPointConstraints, "Constraint the object on a point - setPointConstraints(x, y, z)", void, Vec3d ) },
    //{"setPlaneConstraints", PyWrap(Transform, setPlaneConstraints, "Constraint the object on a plane - setPlaneConstraints(nxf, nyf, nzf)", void ) },
    //{"setAxisConstraints", PyWrap(Transform, setAxisConstraints, "Constraint the object on an axis - TODO -> to test, may work", void ) },
    //{"setRotationConstraints", PyWrap(Transform, setRotationConstraints, "Constraint the object's rotation - setRotationConstraints(xi, yi, zi)", void ) },
    {"setConstraint", PyWrap(Transform, setConstraint, "Set the constraints object - setConstraint( constraint )", void, VRConstraintPtr ) },
    {"getConstraint", PyWrap(Transform, getConstraint, "Get the constraints object - constraint getConstraint()", VRConstraintPtr ) },
    {"physicalize", PyWrap(Transform, physicalize, "physicalize subtree - physicalize( bool physicalized , bool dynamic , str shape, float shape param )\n\tshape can be: ['Box', 'Sphere', 'Convex', 'Concave', 'ConvexDecomposed']", void, bool, bool, string, float ) },
    {"setConvexDecompositionParameters", PyWrap(Transform, setConvexDecompositionParameters, "Set parameters for the convex decomposition, set before physicalize", void, float, float, float, float, float, bool, bool, bool ) },
    {"setGhost", PyWrap(Transform, setGhost, "Set the physics object to be a ghost object", void, bool ) },
    {"attach", PyWrap(Transform, attach, "Attach another object using a joint constraint and a spring constraint", void, VRTransformPtr, VRConstraintPtr, VRConstraintPtr ) },
    {"detachJoint", PyWrap(Transform, detachJoint, "Remove all joints to given transform created by 'attach'", void, VRTransformPtr ) },
    {"setMass", PyWrap(Transform, setMass, "Set the mass of the physics object", void, float ) },
    {"setCollisionMargin", PyWrap(Transform, setCollisionMargin, "Set the collision margin of the physics object", void, float ) },
    {"setCollisionGroup", PyWrap(Transform, setCollisionGroup, "Set the collision groups of the physics object, can be from 0 to 8", void, vector<int> ) },
    {"setCollisionMask", PyWrap(Transform, setCollisionMask, "Set the collision mask of the physics object, can be from 0 to 8 and it is the group to collide with", void, vector<int> ) },
    {"setCollisionShape", PyWrap(Transform, setCollisionShape, "Set the collision shape of the physics object, see physicalize", void, string, float ) },
    {"getCollisions", PyWrap(Transform, getCollisions, "Return the current collisions with other objects", vector<VRCollision> ) },
    {"applyImpulse", PyWrap(Transform, applyImpulse, "Apply impulse on the physics object", void, Vec3d ) },
    {"applyTorqueImpulse", PyWrap(Transform, applyTorqueImpulse, "Apply torque impulse on the physics object", void, Vec3d ) },
    {"applyForce", PyWrap(Transform, applyForce, "Apply force on the physics object", void, Vec3d ) },
    {"applyTorque", PyWrap(Transform, applyTorque, "Apply torque on the physics object ", void, Vec3d ) },
    {"applyConstantForce", PyWrap(Transform, applyConstantForce, "Apply a constant force on the physics object", void, Vec3d ) },
    {"applyConstantTorque", PyWrap(Transform, applyConstantTorque, "Apply a constant torque on the physics object", void, Vec3d ) },
    {"getForce", PyWrap(Transform, getForce, "get the total force put on this transform during this frame", Vec3d ) },
    {"getTorque", PyWrap(Transform, getTorque, "get the total torque put on this transform during this frame", Vec3d ) },
    {"setPhysicsActivationMode", PyWrap(Transform, setPhysicsActivationMode, "Set the physics activation mode of the physics object (normal:1 , no deactivation:4, stay deactivated: 5)", void, int ) },
    {"setPhysicalizeTree", PyWrap(Transform, setPhysicalizeTree, "Set to physicalize whole tree or just current node - setPhysicalizeTree( bool b )", void, bool ) },
    {"animate", PyWrapOpt(Transform, animate, "Animate object along a path, duration [s], offset [s], doOrient, doLoop) )", "1|0", VRAnimationPtr, PathPtr, float, float, bool, bool ) },
    {"getAnimations", PyWrap(Transform, getAnimations, "Return all animations associated to the object", vector<VRAnimationPtr> ) },
    {"stopAnimation", PyWrap(Transform, stopAnimation, "Stop any running animation of this object", void ) },
    {"setGravity", PyWrap(Transform, setGravity, "set Gravity (Vector) of given physicalized object", void, Vec3d ) },
    {"getConstraintAngleWith", PyWrap(Transform, getConstraintAngleWith, "return the relative rotation Angles/position diffs to the given transform (0:rotation, 1:position)", Vec3d, VRTransformPtr, bool ) },
    {"setDamping", PyWrap(Transform, setDamping, "sets the damping of this object. 1st param is the linear, 2nd the angular damping", void, float, float ) },
    {"applyChange", PyWrap(Transform, updateChange, "Apply all changes", void ) },
    {"setCenterOfMass", PyWrap(Transform, setCenterOfMass, "Set a custom center of mass", void, Vec3d ) },
    {"drag", PyWrap(Transform, drag, "Drag this object by new parent", void, VRTransformPtr ) },
    {"drop", PyWrap(Transform, drop, "Drop this object, if held, to old parent", void ) },
    {"rebaseDrag", PyWrap( Transform, rebaseDrag, "Rebase drag, use instead of switchParent", void, VRObjectPtr ) },
    {"isDragged", PyWrap( Transform, isDragged, "Check if transform is beeing dragged", bool ) },
    {"castRay", PyWrap(Transform, intersect, "Cast a ray and return the intersection with given subtree", VRIntersection, VRObjectPtr, Vec3d ) },
    {"getDragParent", PyWrap(Transform, getDragParent, "Get the parent before the drag started", VRObjectPtr ) },
    {"lastChanged", PyWrap(Transform, getLastChange, "Return the frame when the last change occured", uint ) },
    {"setWorldDir", PyWrap(Transform, setWorldDir, "Set the direction in world coordinates", void, Vec3d ) },
    {"setWorldUp", PyWrap(Transform, setWorldUp, "Set the up vector in world coordinates", void, Vec3d ) },
    {"getPoseTo", PyWrap(Transform, getPoseTo, "Get the pose in the coordinate system of another object", PosePtr, VRObjectPtr ) },
    {"applyTransformation", PyWrap(Transform, applyTransformation, "Apply a transformation to the mesh - applyTransformation( pose )", void ) },
    {NULL}  /* Sentinel */
};

PyObject* VRPyTransform::fromSharedPtr(VRTransformPtr obj) {
    return VRPyTypeCaster::cast(dynamic_pointer_cast<VRObject>(obj));
}

/*PyObject* VRPyTransform::applyTransformation(VRPyTransform* self, PyObject *args) {
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
    c->setActive(true);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setPlaneConstraints(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d v = parseVec3d(args);
    auto c = self->objPtr->getConstraint();
    c->setTConstraint(v, OSG::VRConstraint::PLANE);
    c->setActive(true);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setAxisConstraints(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d v = parseVec3d(args);
    auto c = self->objPtr->getConstraint();
    c->setTConstraint(v, OSG::VRConstraint::LINE);
    c->setActive(true);
    Py_RETURN_TRUE;
}

PyObject* VRPyTransform::setRotationConstraints(VRPyTransform* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec3d v = parseVec3d(args);
    auto c = self->objPtr->getConstraint();
    c->setRConstraint(self->objPtr->getWorldPosition(), OSG::VRConstraint::POINT);
    c->setActive(true);
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
    VRPyTransform* t;
    if(self->objPtr->getPhysics()->isSoft()) {
        int nodeIndex;
        int ignoreCollision;
        float influence;
        PyObject* localPiv;
        if (! PyArg_ParseTuple(args, "OiOif", &t, &nodeIndex, &localPiv, &ignoreCollision, &influence)) return NULL;
        self->objPtr->getPhysics()->setConstraint(t->objPtr->getPhysics(), nodeIndex, parseVec3dList(localPiv), ignoreCollision, influence);
    }
    else {
        VRPyTransform* t;
        VRPyConstraint* c;
        VRPyConstraint* cs = 0;
        if (! PyArg_ParseTuple(args, "OO|O", &t, &c, &cs)) return NULL;
        VRConstraintPtr csc = 0;
        if (cs) csc = cs->objPtr;
        self->objPtr->getPhysics()->setConstraint( t->objPtr->getPhysics(), c->objPtr, csc );
        //t->objPtr->attach(self->objPtr, c->objPtr);
        self->objPtr->attach(t->objPtr, c->objPtr);
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
}*/
