#include "VRPyTransform.h"
#include "VRPyConstraint.h"
#include "VRPyAnimation.h"
#include "VRPyPose.h"
#include "VRPyPath.h"
#include "VRPyIntersection.h"
#include "VRPyTypeCaster.h"
#include "VRPyMath.h"
#include "VRPyBaseT.h"
#include "core/objects/VRAnimation.h"
#include "core/math/kinematics/VRConstraint.h"
#include "core/setup/devices/VRIntersect.h"
#ifndef WITHOUT_BULLET
#include "core/objects/geometry/VRPhysics.h"
#endif

using namespace OSG;

#ifndef WITHOUT_BULLET
simpleVRPyType(Collision, 0);

template<> PyObject* VRPyTypeCaster::cast(const VRCollision& e) { return VRPyCollision::fromObject(e); }

PyMethodDef VRPyCollision::methods[] = {
    {"getPos1", PyWrap(Collision, getPos1, "Get the first collision point", Vec3d) },
    {"getPos2", PyWrap(Collision, getPos2, "Get the second collision point", Vec3d) },
    {"getNorm", PyWrap(Collision, getNorm, "Get collision normal vector", Vec3d) },
    {"getDistance", PyWrap(Collision, getDistance, "Get the distance between collision points", float) },
    {"getObj1", PyWrap(Collision, getObj1, "Get the first collision object", VRTransformPtr) },
    {"getObj2", PyWrap(Collision, getObj2, "Get the second collision object", VRTransformPtr) },
    {"getTriangle1", PyWrap(Collision, getTriangle1, "Get the first collision triangle", vector<Vec4d>) },
    {"getTriangle2", PyWrap(Collision, getTriangle2, "Get the second collision triangle", vector<Vec4d>) },
    {NULL}  /* Sentinel */
};
#endif

simpleVRPyType(Transform, New_VRObjects_ptr);

PyMethodDef VRPyTransform::methods[] = {
    {"setIdentity", PyWrap(Transform, setIdentity, "Reset transformation to identity", void ) },
    {"translate", PyWrap(Transform, translate, "Translate the object along a vector", void, Vec3d ) },
    {"move", PyWrap(Transform, move, "Move the object in dir direction", void, float ) },
    {"rotate", PyWrapOpt(Transform, rotate, "Rotate the object around an axis and angle", "0 0 0", void, float, Vec3d, Vec3d ) },
    {"rotateWorld", PyWrapOpt(Transform, rotateWorld, "Rotate the object around an axis and angle", "0 0 0", void, float, Vec3d, Vec3d ) },
    {"rotateAround", PyWrapOpt(Transform, rotateAround, "Rotate the object around its at axis", "0 1 0", void, float, Vec3d ) },
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
    {"getWorldScale", PyWrapOpt(Transform, getWorldScale, "Return the object's scale vector", "0", Vec3d, bool ) },
    {"getScale", PyWrap(Transform, getScale, "Return the object's scale vector", Vec3d ) },
    {"getEuler", PyWrap(Transform, getEuler, "Return the object's euler angles", Vec3d ) },
    {"setMatrix", PyWrap(Transform, setMatrix, "Set the object's matrix", void, Matrix4d ) },
    {"setWorldMatrix", PyWrap(Transform, setWorldMatrix, "Set the object's world matrix", void, Matrix4d ) },
    {"setWorldFrom", PyWrap(Transform, setWorldPosition, "Set the object's world position", void, Vec3d ) },
    {"setWorldOrientation", PyWrap(Transform, setWorldOrientation, "Set the object's world direction", void, Vec3d, Vec3d ) },
    {"setWorldUp", PyWrap(Transform, setWorldUp, "Set the object's up vector", void, Vec3d ) },
    {"setWorldAt", PyWrap(Transform, setWorldAt, "Set the object's at vector", void, Vec3d ) },
    {"setWorldScale", PyWrap(Transform, setWorldScale, "Set the object's scale vector", void, Vec3d ) },
    {"setPose", PyWrap(Transform, setPose, "Set the object's pose", void, PosePtr ) },
    {"setTransform", PyWrapOpt(Transform, setTransform, "Set the object's pose", "0 0 -1|0 1 0", void, Vec3d, Vec3d, Vec3d ) },
    {"setOrientation", PyWrapOpt(Transform, setOrientation, "Set the object's orientation", "0 1 0", void, Vec3d, Vec3d ) },
    {"setOrientationQuat", PyWrap(Transform, setOrientationQuat, "Set the object's orientation input takes quaternion", void, Vec4d ) },
    {"setWorldPose", PyWrap(Transform, setWorldPose, "Set the object's pose", void, PosePtr ) },
    {"setPosition", PyWrap(Transform, setFrom, "Set the object's from vector", void, Vec3d ) },
    {"setFrom", PyWrap(Transform, setFrom, "Set the object's from vector", void, Vec3d ) },
    {"setAt", PyWrap(Transform, setAt, "Set the object's at vector", void, Vec3d ) },
    {"setDir", PyWrap(Transform, setDir, "Set the object's dir vector", void, Vec3d ) },
    {"setEuler", PyWrap(Transform, setEuler, "Set the object's orientation using Euler angles in radians", void, Vec3d ) },
    {"setEulerDegree", PyWrap(Transform, setEulerDegree, "Set the object's orientation using Euler angles in degrees", void, Vec3d ) },
    {"setUp", PyWrap(Transform, setUp, "Set the object's up vector", void, Vec3d ) },
    {"setScale", PyWrap(Transform, setScale, "Set the object's scale vector", void, Vec3d ) },
    //{"setPointConstraints", PyWrap(Transform, setPointConstraints, "Constraint the object on a point - setPointConstraints(x, y, z)", void, Vec3d ) },
    //{"setPlaneConstraints", PyWrap(Transform, setPlaneConstraints, "Constraint the object on a plane - setPlaneConstraints(nxf, nyf, nzf)", void ) },
    //{"setAxisConstraints", PyWrap(Transform, setAxisConstraints, "Constraint the object on an axis - TODO -> to test, may work", void ) },
    //{"setRotationConstraints", PyWrap(Transform, setRotationConstraints, "Constraint the object's rotation - setRotationConstraints(xi, yi, zi)", void ) },
    {"setConstraint", PyWrap(Transform, setConstraint, "Set the constraints object - setConstraint( constraint )", void, VRConstraintPtr ) },
    {"getConstraint", PyWrap(Transform, getConstraint, "Get the constraints object - constraint getConstraint()", VRConstraintPtr ) },
    {"attach", PyWrapOpt(Transform, attach, "Attach another object using a joint constraint and a spring constraint", "0", void, VRTransformPtr, VRConstraintPtr, VRConstraintPtr ) },
    {"detachJoint", PyWrap(Transform, detachJoint, "Remove all joints to given transform created by 'attach'", void, VRTransformPtr ) },
    {"setSpringParameters", PyWrap(Transform, setSpringParameters, "Set joint spring parameters", void, VRTransformPtr, int, float, float ) },
    {"animate", PyWrapOpt(Transform, animate, "Animate object along a path, duration [s], offset [s], doOrient, doLoop) )", "1|0|0", VRAnimationPtr, PathPtr, float, float, bool, bool, PathPtr ) },
    {"getAnimations", PyWrap(Transform, getAnimations, "Return all animations associated to the object", vector<VRAnimationPtr> ) },
    {"stopAnimation", PyWrap(Transform, stopAnimation, "Stop any running animation of this object", void ) },
    {"getConstraintAngleWith", PyWrap(Transform, getConstraintAngleWith, "return the relative rotation Angles/position diffs to the given transform (0:rotation, 1:position)", Vec3d, VRTransformPtr, bool ) },
    {"applyChange", PyWrap(Transform, updateChange, "Apply all changes", void ) },
    {"drag", PyWrapOpt(Transform, drag, "Drag this object by new parent", "", void, VRTransformPtr, VRIntersection ) },
    {"drop", PyWrap(Transform, drop, "Drop this object, if held, to old parent", void ) },
    {"rebaseDrag", PyWrap( Transform, rebaseDrag, "Rebase drag, use instead of switchParent", void, VRObjectPtr ) },
    {"isDragged", PyWrap( Transform, isDragged, "Check if transform is beeing dragged", bool ) },
    {"castRay", PyWrap(Transform, intersect, "Cast a ray and return the intersection with given subtree", VRIntersection, VRObjectPtr, Vec3d ) },
    {"getDragParent", PyWrap(Transform, getDragParent, "Get the parent before the drag started", VRObjectPtr ) },
    {"lastChanged", PyWrap(Transform, getLastChange, "Return the frame when the last change occured", unsigned int ) },
    {"changedSince", PyWrapOpt(Transform, changedSince2, "Check if change occured since frame, flag includes frame", "1", bool, unsigned int, bool ) },
    {"setWorldDir", PyWrap(Transform, setWorldDir, "Set the direction in world coordinates", void, Vec3d ) },
    {"setWorldUp", PyWrap(Transform, setWorldUp, "Set the up vector in world coordinates", void, Vec3d ) },
    {"applyTransformation", PyWrapOpt(Transform, applyTransformation, "Apply a transformation to the mesh - applyTransformation( pose )", "0", void, PosePtr ) },
#ifndef WITHOUT_BULLET
    {"physicalize", PyWrapOpt(Transform, physicalize, "physicalize subtree - physicalize( bool physicalized , bool dynamic , str shape, float shape param )\n\tshape can be: ['Box', 'Sphere', 'Convex', 'Concave', 'ConvexDecomposed']", "0", void, bool, bool, string, float ) },
    {"setConvexDecompositionParameters", PyWrap(Transform, setConvexDecompositionParameters, "Set parameters for the convex decomposition, set before physicalize: compacityWeight (0.1), volumeWeight (0.0), NClusters (2), NVerticesPerCH (100), concavity (100), addExtraDistPoints (false), addNeighboursDistPoints (false), addFacesPoints (false) ", void, float, float, float, float, float, bool, bool, bool ) },
    {"setGhost", PyWrap(Transform, setGhost, "Set the physics object to be a ghost object", void, bool ) },
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
    {"setGravity", PyWrap(Transform, setGravity, "set Gravity (Vector) of given physicalized object", void, Vec3d ) },
    {"setDamping", PyWrap(Transform, setDamping, "sets the damping of this object. 1st param is the linear, 2nd the angular damping", void, float, float ) },
    {"setCenterOfMass", PyWrap(Transform, setCenterOfMass, "Set a custom physics center of mass offset", void, Vec3d ) },
    {"getCenterOfMass", PyWrap(Transform, getCenterOfMass, "get physics center of mass offset", Vec3d ) },
    {"getPhysicsDynamic", PyWrap(Transform, getPhysicsDynamic, "get if dynamic physics object", bool ) },
    {"setPhysicsDynamic", PyWrap(Transform, setPhysicsDynamic, "set if dynamic physics object", void, bool ) },
#endif
    {NULL}  /* Sentinel */
};

PyObject* VRPyTransform::fromSharedPtr(VRTransformPtr obj) {
    return VRPyTypeCaster::cast(dynamic_pointer_cast<VRObject>(obj));
}







