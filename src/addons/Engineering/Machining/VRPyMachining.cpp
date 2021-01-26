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
    {"stop", PyWrap(MachiningSimulation, stop, "Start simulation, optional speed multiplier", void) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyMachiningCode::methods[] = {
    {"readGCode", PyWrap(MachiningCode, readGCode, "Read G code from file, appends instructions, path and speedMultiplier", void, string, double) },
    {"clear", PyWrap(MachiningCode, clear, "Clear instructions", void) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyMachiningKinematics::methods[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyCartesianKinematics::methods[] = {
    {"setComponents", PyWrap(CartesianKinematics, setComponents, "Set kinematic components", void, VRTransformPtr, VRTransformPtr, VRTransformPtr) },
    {NULL}  /* Sentinel */
};
