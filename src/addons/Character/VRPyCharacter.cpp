#include "VRPyCharacter.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Behavior, New_ptr);
simpleVRPyType(Skeleton, New_ptr);
simpleVRPyType(Character, New_VRObjects_ptr);


PyMethodDef VRPyBehavior::methods[] = {
    {NULL}  /* Sentinel */
};

typedef map<int,Vec3d> vectorMap;

PyMethodDef VRPySkeleton::methods[] = {
    {"getJointsPositions", PyWrap( Skeleton, getJointsPositions, "Get all skeleton joints positions", vectorMap ) },
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
    {NULL}  /* Sentinel */
};




