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
    {"getVehicleData", PyWrap( TrafficSimulation, getVehicleData, "getVehicleData", string, int ) },
    {"getEdgeData", PyWrap( TrafficSimulation, getEdgeData, "getEdgeData", string, int ) },
    {"setSeedRoad", PyWrap( TrafficSimulation, setSeedRoad, "setSeedRoad", void, int ) },
    {"setSeedRoadVec", PyWrap( TrafficSimulation, setSeedRoadVec, "setSeedRoadVec", void, vector<int> ) },
    {"runDiagnostics", PyWrap( TrafficSimulation, runDiagnostics, "runDiagnostics", void ) },
    {"isSeedRoad", PyWrap( TrafficSimulation, isSeedRoad, "isSeedRoad", bool, int ) },
    {"forceIntention", PyWrap( TrafficSimulation, forceIntention, "forceIntention", void, int, int ) },
    {"runWithoutGeometries", PyWrap( TrafficSimulation, runWithoutGeometries, "runWithoutGeometries", void ) },
    {"runWithGeometries", PyWrap( TrafficSimulation, runWithGeometries, "runWithGeometries", void ) },
    {"showGraph", PyWrap( TrafficSimulation, showGraph, "showGraph", void ) },
    {"hideGraph", PyWrap( TrafficSimulation, hideGraph, "hideGraph", void ) },
    {"showIntersections", PyWrap( TrafficSimulation, showIntersections, "showIntersections", void ) },
    {"hideIntersections", PyWrap( TrafficSimulation, hideIntersections, "hideIntersections", void ) },
    {"toggleLaneChanges", PyWrap( TrafficSimulation, toggleLaneChanges, "toggleLaneChanges", void ) },
    {"showVehicVision", PyWrap( TrafficSimulation, showVehicVision, "showVehicVision", void ) },
    {"hideVehicVision", PyWrap( TrafficSimulation, hideVehicVision, "hideVehicVision", void ) },
    {"stopVehicle", PyWrap( TrafficSimulation, stopVehicle, "stopVehicle", void, int ) },
    {NULL}  /* Sentinel */
};
