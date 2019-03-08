#include "VRPyTrafficSimulation.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(TrafficSimulation, New_ptr);

PyMethodDef VRPyTrafficSimulation::methods[] = {
    {"setRoadNetwork", PyWrap( TrafficSimulation, setRoadNetwork, "Set road network", void, VRRoadNetworkPtr ) },
    {"setTrafficDensity", PyWrapOpt( TrafficSimulation, setTrafficDensity, "Set overall traffic - density, type, maxUnits, maxTransforms", "0", void, float, int, int, int ) },
    {"addVehicleModel", PyWrap( TrafficSimulation, addVehicleModel, "Set vehicle geometry", int, VRObjectPtr ) },
    {"addUser", PyWrap( TrafficSimulation, addUser, "Set vehicle geometry", void, VRTransformPtr ) },
    {"getUserCollisionState", PyWrap( TrafficSimulation, getUserCollisionState, "getUserCollisionState: int input = userID", bool, int ) },
    {"saveSim", PyWrap( TrafficSimulation, saveSim, "saves Simulation as file", void, string ) },
    {"loadSim", PyWrap( TrafficSimulation, loadSim, "loads Simulation from file", void, string ) },
    ///Diagnostics
    {"getUser", PyWrap( TrafficSimulation, getUser, "Get first user vehicle geometry", VRTransformPtr ) },
    {"toggleSim", PyWrap( TrafficSimulation, toggleSim, "toggleSim - pause/resume", void, void ) },
    {"toggleSimUpd", PyWrap( TrafficSimulation, toggleSimUpd, "toggleSim - pause/resume geometry updating", void, void ) },
    {"toggleVisibility", PyWrap( TrafficSimulation, toggleVisibility, "toggles Visibilty", void, void ) },
    {"toggleDirection", PyWrap( TrafficSimulation, toggleDirection, "toggleDirection - forward/backward", void, void ) },
    {"setSpeedmultiplier", PyWrap( TrafficSimulation, setSpeedmultiplier, "setSpeedmultiplier", void, float ) },
    {"setGlobalOffset", PyWrap( TrafficSimulation, setGlobalOffset, "setGlobalOffset - if trafficSim is located somewhere else as user", void, Vec3d ) },
    {"getVehicleData", PyWrap( TrafficSimulation, getVehicleData, "getVehicleData", string, int ) },
    {"getEdgeData", PyWrap( TrafficSimulation, getEdgeData, "getEdgeData", string, int ) },
    {"setSeedRoad", PyWrap( TrafficSimulation, setSeedRoad, "setSeedRoad", void, int ) },
    {"setSeedRoadVec", PyWrap( TrafficSimulation, setSeedRoadVec, "setSeedRoadVec", void, vector<int> ) },
    {"runDiagnostics", PyWrap( TrafficSimulation, runDiagnostics, "runDiagnostics", void ) },
    {"runVehicleDiagnostics", PyWrap( TrafficSimulation, runVehicleDiagnostics, "runVehicleDiagnostics", void ) },
    //{"isSeedRoad", PyWrap( TrafficSimulation, isSeedRoad, "isSeedRoad", bool, int ) },
    {"forceIntention", PyWrap( TrafficSimulation, forceIntention, "forceIntention (int vID,int behavior)", void, int, int ) },
    {"toggleGraph", PyWrap( TrafficSimulation, toggleGraph, "enables/disables visualisation of: roadnetwork graph", void ) },
    {"toggleVehicMarkers", PyWrap( TrafficSimulation, toggleVehicMarkers, "enables/disables visualisation of: next Intersection in vehicle sight", void, int ) },
    {"toggleIntersections", PyWrap( TrafficSimulation, toggleIntersections, "enables/disables visualisation of: intersection location and type", void ) },
    {"toggleLaneChanges", PyWrap( TrafficSimulation, toggleLaneChanges, "toggleLaneChanges", void ) },
    {"toggleVehicVision", PyWrap( TrafficSimulation, toggleVehicVision, "enables/disables visualisation of: vehicle vision", void ) },
    {"stopVehicle", PyWrap( TrafficSimulation, stopVehicle, "stopVehicle", void, int ) },
    {"setKillswitches", PyWrap( TrafficSimulation, setKillswitches, "setKillswitches", void, float, float ) },
    {"setVisibilityRadius", PyWrap( TrafficSimulation, setVisibilityRadius, "setVisibilityRadius", void, float ) },
    {"addDcar", PyWrap( TrafficSimulation, addDcar, "addDcar", void, int ) },
    //{"deleteVehicle", PyWrap( TrafficSimulation, deleteVehicle, "deleteVehicle", void, int ) },
    {NULL}  /* Sentinel */
};
