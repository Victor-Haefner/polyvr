#include "VRPyMachining.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(MachiningCode, New_ptr);
simpleVRPyType(CartesianKinematics, New_ptr);

PyMethodDef VRPyMachiningCode::methods[] = {
    {"readGCode", PyWrap(MachiningCode, readGCode, "Read G code from file", void, string) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyCartesianKinematics::methods[] = {
    {NULL}  /* Sentinel */
};