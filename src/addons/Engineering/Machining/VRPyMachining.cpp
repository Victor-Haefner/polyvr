#include "VRPyMachining.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(MachiningSimulation, New_ptr);
simpleVRPyType(MachiningCode, New_ptr);
simpleVRPyType(MachiningKinematics, 0);
simpleVRPyType(CartesianKinematics, New_ptr);

PyMethodDef VRPyMachiningSimulation::methods[] = {
    {"setKinematics", PyWrap(MachiningSimulation, setKinematics, "Set kinematics model", void, VRMachiningKinematicsPtr) },
    {"setCode", PyWrap(MachiningSimulation, setCode, "Set machining code", void, VRMachiningCodePtr) },
    {"setOnFinish", PyWrap(MachiningSimulation, setOnFinish, "Set callback to trigger when finished", void, VRUpdateCbPtr) },
    {"start", PyWrapOpt(MachiningSimulation, start, "Start simulation, optional speed multiplier", "1", void, double) },
    {"stop", PyWrap(MachiningSimulation, stop, "Stop simulation", void) },
    {"pause", PyWrap(MachiningSimulation, pause, "Pause/unpause simulation", void, bool) },
    {"isPaused", PyWrap(MachiningSimulation, isPaused, "Returns if sim paused", bool) },
    {"setOrigin", PyWrap(MachiningSimulation, setOrigin, "Set Origin", void, Vec3d) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyMachiningCode::methods[] = {
    {"readGCode", PyWrap(MachiningCode, readGCode, "Read G code from file, appends instructions, path and speedMultiplier. ", void, string, double) },
    {"parseFile", PyWrapOpt(MachiningCode, parseFile, "parse an individual file, call before readGCode", "0", void, string, bool) },
    {"parseFolder", PyWrap(MachiningCode, parseFolder, "parse all files in a folder, call before readGCode", void, string) },
    {"clear", PyWrap(MachiningCode, clear, "Clear instructions", void) },
    {"asGeometry", PyWrap(MachiningCode, asGeometry, "To plot the simulation of the given G Code", VRGeometryPtr) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyMachiningKinematics::methods[] = {
    {"setEndEffector", PyWrap(MachiningKinematics, setEndEffector, "Set end effector position", void, PosePtr) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyCartesianKinematics::methods[] = {
    {"setComponents", PyWrap(CartesianKinematics, setComponents, "Set kinematic components", void, VRTransformPtr, VRTransformPtr, VRTransformPtr) },
    {"setAxisParams", PyWrap(CartesianKinematics, setAxisParams, "Set axis parameters", void, int, int, int) },
    {NULL}  /* Sentinel */
};
