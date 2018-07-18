#include "VRPyTrafficSimulation.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(TrafficSimulation, New_ptr);

PyMethodDef VRPyTrafficSimulation::methods[] = {
    {"setRoadNetwork", PyWrap( TrafficSimulation, setRoadNetwork, "Set road network", void, VRRoadNetworkPtr ) },
    {"setTrafficDensity", PyWrapOpt( TrafficSimulation, setTrafficDensity, "Set overall traffic", "0", void, float, int, int ) },
    {"addVehicleModel", PyWrap( TrafficSimulation, addVehicleModel, "Set vehicle geometry", int, VRObjectPtr ) },
    {"addUser", PyWrap( TrafficSimulation, addUser, "Set vehicle geometry", void, VRTransformPtr ) },
    {"toggleSim", PyWrap( TrafficSimulation, toggleSim, "toggleSim - pause/resume", void, void ) },
    {"setSpeedmultiplier", PyWrap( TrafficSimulation, setSpeedmultiplier, "setSpeedmultiplier", void, float ) },
    {NULL}  /* Sentinel */
};
