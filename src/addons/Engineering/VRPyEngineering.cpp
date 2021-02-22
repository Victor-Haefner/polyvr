#include "VRPyEngineering.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyTypeCaster.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyPath.h"

using namespace OSG;

simpleVRPyType(NumberingEngine, New_VRObjects_ptr);
simpleVRPyType(RobotArm, New_named_ptr);
simpleVRPyType(PipeSystem, New_ptr);

PyMethodDef VRPyPipeSystem::methods[] = {
    {NULL}
};

PyMethodDef VRPyNumberingEngine::methods[] = {
    {"set", PyWrapOpt( NumberingEngine, set, "Set number: ID, pos, number, decimal places, groupID", "2|0", void, int, Vec3d, float, int, int ) },
    {"clear", PyWrap( NumberingEngine, clear, "Clear numbers", void ) },
    {NULL}
};

PyMethodDef VRPyRobotArm::methods[] = {
    {"showAnalytics", PyWrap(RobotArm, showAnalytics, "Shows a visualization of the analytic model", void, bool ) },
    {"setParts", PyWrap(RobotArm, setParts, "Set robot parts", void, vector<VRTransformPtr> ) },
    {"setAngleOffsets", PyWrap(RobotArm, setAngleOffsets, "Set angle offset for each part", void, vector<float> ) },
    {"setAngleDirections", PyWrap(RobotArm, setAngleDirections, "Set angles rotation direction - setAngleDirections([1/-1])", void, vector<int> ) },
    {"setAxis", PyWrap(RobotArm, setAxis, "Set rotation axis for each part - setAxis([int a])\n a: 0 = 'x', 1 = 'y', 2 = 'z'", void, vector<int> ) },
    {"setLengths", PyWrap(RobotArm, setLengths, "Set kinematic lengths between joints - setLengths([base_height, upper_arm length, forearm length, grab position])", void, vector<float> ) },
    {"setMaxSpeed", PyWrap(RobotArm, setMaxSpeed, "Set max angular speed", void, float ) },
    {"moveTo", PyWrap(RobotArm, moveTo, "Move the end effector to a certain position - moveTo([x,y,z])", void, PosePtr ) },
    {"setGrab", PyWrap(RobotArm, setGrab, "Set grab state - setGrab(float d)\n d: 0 is closed, 1 is open", void, float ) },
    {"toggleGrab", PyWrap(RobotArm, toggleGrab, "Toggle the grab - toggleGrab()", void ) },
    {"grab", PyWrap(RobotArm, grab, "Grab object", void, VRTransformPtr ) },
    {"drop", PyWrap(RobotArm, drop, "Drop held object", void ) },
    {"setAngles", PyWrapOpt(RobotArm, setAngles, "Set joint angles - setAngles( angles, force )", "0", void, vector<float>, bool ) },
    {"getAngles", PyWrap(RobotArm, getAngles, "Get joint angles - getAngles()", vector<float> ) },
    //{"getForwardKinematics", PyWrap(RobotArm, getForwardKinematics, "Get end effector pose from angles - p,d,u getForwardKinematics( angles )") },
    //{"getBackwardKinematics", PyWrap(RobotArm, getBackwardKinematics, "Get angles from end effector pose - angles getBackwardKinematics( p,d,u )") },
    {"setPath", PyWrapOpt(RobotArm, setPath, "Set robot path(s) - second path is optinal and overrides orientation", "0", void, PathPtr, PathPtr ) },
    {"getPath", PyWrap(RobotArm, getPath, "Get robot path", PathPtr ) },
    {"getOrientationPath", PyWrap(RobotArm, getOrientationPath, "Get robot orientation path", PathPtr ) },
    {"getParts", PyWrap(RobotArm, getParts, "Get robot parts", vector<VRTransformPtr> ) },
    {"moveOnPath", PyWrapOpt(RobotArm, moveOnPath, "Move robot on internal path - moveOnPath(t0, t1, doLoop, durationMultiplier)", "0|1", void, float, float, bool, float) },
    {"isMoving", PyWrap(RobotArm, isMoving, "Get animation status", bool) },
    {"setEventCallback", PyWrap(RobotArm, setEventCallback, "Set callback for move and stop events", void, VRMessageCbPtr) },
    {NULL}  /* Sentinel */
};
