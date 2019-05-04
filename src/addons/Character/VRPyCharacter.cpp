#include "VRPyCharacter.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyPose.h"
#include "core/scripting/VRPyMath.h"
#include "core/scripting/VRPyConstraint.h"

using namespace OSG;

template<> PyObject* VRPyTypeCaster::cast(const VRSkeleton::EndEffector& e) {
    PyObject* epy = PyTuple_New(3);
    PyTuple_SetItem(epy, 0, PyString_FromString(e.name.c_str()));
    PyTuple_SetItem(epy, 1, PyInt_FromLong(e.boneID));
    PyTuple_SetItem(epy, 2, VRPyPose::fromSharedPtr(e.target));
    return epy;
}

template<> PyObject* VRPyTypeCaster::cast(const VRSkeleton::Joint& e) {
    cout << "VRPyTypeCaster::cast " << e.pos << endl;
    PyObject* epy = PyTuple_New(5);
    PyTuple_SetItem(epy, 0, PyString_FromString(e.name.c_str()));
    PyTuple_SetItem(epy, 1, VRPyVec3f::fromVector(e.pos));
    PyTuple_SetItem(epy, 2, PyInt_FromLong(e.bone1));
    PyTuple_SetItem(epy, 3, PyInt_FromLong(e.bone2));
    PyTuple_SetItem(epy, 4, VRPyConstraint::fromSharedPtr(e.constraint));
    return epy;
}

simpleVRPyType(Behavior, New_ptr);
simpleVRPyType(Skeleton, New_ptr);
simpleVRPyType(Character, New_VRObjects_ptr);


PyMethodDef VRPyBehavior::methods[] = {
    {NULL}  /* Sentinel */
};

typedef map<int,Vec3d> vectorMap;
typedef map<string, VRSkeleton::EndEffector> eeMap;

PyMethodDef VRPySkeleton::methods[] = {
    {"getJointsPositions", PyWrap( Skeleton, getJointsPositions, "Get all skeleton joints positions", vectorMap ) },
    {"getEndEffectors", PyWrap( Skeleton, getEndEffectors, "Get end effectors", eeMap ) },
    {"getChain", PyWrap( Skeleton, getChain, "Get chain of joints to end effector", vector<VRSkeleton::Joint>, string ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyCharacter::methods[] = {
    {"setSkeleton", PyWrap( Character, setSkeleton, "Set the skeleton", void, VRSkeletonPtr ) },
    {"getSkeleton", PyWrap( Character, getSkeleton, "Get the skeleton", VRSkeletonPtr ) },
    //{"setSkin", PyWrap( Character, setSkin, "Set the skin geometry", void, string, bool ) },
    {"addBehavior", PyWrap( Character, addBehavior, "Add a behavior pattern", void, VRBehaviorPtr ) },
    //{"triggerBehavior", PyWrap( Character, triggerBehavior, "Trigger a certain behavior", void, string ) },
    {"simpleSetup", PyWrap( Character, simpleSetup, "Simple character setup", void ) },
    {"move", PyWrap( Character, move, "Move end effector, 'handLeft', 'handRight', footLeft', 'footRight'", void, string, PosePtr ) },
    {"overrideSim", PyWrap( Character, overrideSim, "Override kinematics simulation", void, VRUpdateCbPtr ) },
    {NULL}  /* Sentinel */
};




