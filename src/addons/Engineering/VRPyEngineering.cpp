#include "VRPyEngineering.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyTypeCaster.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyPath.h"

using namespace OSG;

simpleVRPyType(NumberingEngine, New_VRObjects_ptr);
simpleVRPyType(RobotArm, New_named_ptr);
simpleVRPyType(PipeSystem, New_ptr);
simpleVRPyType(ElectricSystem, New_ptr);
simpleVRPyType(ElectricVisualization, New_ptr);
simpleVRPyType(Wire, 0);
simpleVRPyType(WiringSimulation, 0);
simpleVRPyType(ElectricComponent, 0);
simpleVRPyType(LADVariable, New_ptr);
simpleVRPyType(LADEngine, New_ptr);
simpleVRPyType(SCLScript, New_ptr);
simpleVRPyType(SCLEngine, New_ptr);
simpleVRPyType(RocketExhaust, New_VRObjects_ptr);
simpleVRPyType(SpaceMission, New_ptr);

template<> PyObject* VRPyTypeCaster::cast(const VRElectricComponent::Address& a) {
    map<string, string> v;
    v["address"] = a.address;
    v["ecadID"] = a.ecadID;
    v["port"] = a.port;
    v["component"] = a.component;
    v["socket"] = a.socket;
    v["machine"] = a.machine;
    return VRPyTypeCaster::cast(v);
}

PyMethodDef VRPyPipeSystem::methods[] = {
    {"addNode", PyWrap( PipeSystem, addNode, "Add node, type can be [Tank, Valve, Outlet, Pump]", int, string, PosePtr, string, map<string, string> ) },
    {"remNode", PyWrap( PipeSystem, remNode, "Remove node", void, int ) },
    {"addSegment", PyWrap( PipeSystem, addSegment, "Add segment between nodes (radius, n1, n2)", int, double, int, int ) },
    {"remSegment", PyWrap( PipeSystem, remSegment, "Remove segment", void, int ) },
    {"setFlowParameters", PyWrap( PipeSystem, setFlowParameters, "Set flow parameters, (latency)", void, float ) },
    {"setDoVisual", PyWrapOpt( PipeSystem, setDoVisual, "Enable visual", "0.1", void, bool, float ) },
    {"setNodePose", PyWrap( PipeSystem, setNodePose, "Set node pose by ID", void, int, PosePtr ) },
    {"disconnect", PyWrap( PipeSystem, disconnect, "Disconnect a node from a segment, keeps the segment by adding a junction to its end (nId, sID)", int, int, int ) },
    {"insertSegment", PyWrap( PipeSystem, insertSegment, "Insert a segment between a node and segment (nID, sID, radius)", int, int, int, float ) },
    {"getGraph", PyWrap( PipeSystem, getGraph, "Get internal graph", GraphPtr ) },
    {"getOntology", PyWrap( PipeSystem, getOntology, "Get ontology", VROntologyPtr ) },
    {"getNode", PyWrap( PipeSystem, getNode, "Get node ID by name", int, string ) },
    {"getNodeEntity", PyWrap( PipeSystem, getNodeEntity, "Get node entity by node ID", VREntityPtr, int ) },
    {"getNodeName", PyWrap( PipeSystem, getNodeName, "Get node name", string, int ) },
    {"getNodePose", PyWrap( PipeSystem, getNodePose, "Get node pose", PosePtr, int ) },
    {"getSegment", PyWrap( PipeSystem, getSegment, "Get segment ID by its node IDs", int, int, int ) },
    {"getSegmentPressure", PyWrap( PipeSystem, getSegmentPressure, "Get segment pressure", double, int ) },
    {"getSegmentGradient", PyWrap( PipeSystem, getSegmentGradient, "Get segment pressure gradient", Vec2d, int ) },
    {"getSegmentDensity", PyWrap( PipeSystem, getSegmentDensity, "Get segment density", double, int ) },
    {"getSegmentFlow", PyWrap( PipeSystem, getSegmentFlow, "Get segment flow", double, int ) },
    {"getValveState", PyWrap( PipeSystem, getValveState, "Get valve state", bool, string ) },
    {"getSegmentFlowAccelleration", PyWrap( PipeSystem, getSegmentFlowAccelleration, "Get segment flow acceleration due to pressure gradient", Vec2d, int ) },
    {"getTankPressure", PyWrap( PipeSystem, getTankPressure, "Get tank pressure", double, string ) },
    {"getTankDensity", PyWrap( PipeSystem, getTankDensity, "Get tank density", double, string ) },
    {"getTankVolume", PyWrap( PipeSystem, getTankVolume, "Get tank volume", double, string ) },
    {"getPump", PyWrap( PipeSystem, getPump, "Get pump performance", double, string ) },
    {"setPump", PyWrap( PipeSystem, setPump, "Set pump performance and max pressure", void, string, double, double ) },
    {"setValve", PyWrap( PipeSystem, setValve, "Set valve state", void, string, bool ) },
    {"setTankPressure", PyWrap( PipeSystem, setTankPressure, "Set tank pressure", void, string, double ) },
    {"setTankDensity", PyWrap( PipeSystem, setTankDensity, "Set tank density", void, string, double ) },
    {"setPipeRadius", PyWrap( PipeSystem, setPipeRadius, "Set pipe radius, set to 0 to simulate blocked pipe", void, int, double ) },
    {"setOutletDensity", PyWrap( PipeSystem, setOutletDensity, "Set outlet exterior density", void, string, double ) },
    {"setOutletPressure", PyWrap( PipeSystem, setOutletPressure, "Set outlet exterior pressure", void, string, double ) },
    {"printSystem", PyWrap( PipeSystem, printSystem, "Print system state to console", void ) },
    {"updateInspection", PyWrap( PipeSystem, updateInspection, "Visualize node information", void, int ) },
    {NULL}
};

typedef map<string, VRLADVariablePtr> strLadMap;
typedef map<string, VRObjectPtr> strObjMap;
typedef map<size_t, VRElectricComponentPtr> intECompMap;
typedef map<string, vector<VRElectricComponentPtr>> strVecECompMap;

PyMethodDef VRPyElectricSystem::methods[] = {
    {"newComponent", PyWrap( ElectricSystem, newComponent, "Add new component", VRElectricComponentPtr, string, string, string ) },
    {"addVariable", PyWrap( ElectricSystem, addVariable, "Add new variable", void, string, VRLADVariablePtr ) },
    {"importEPLAN", PyWrap( ElectricSystem, importEPLAN, "Import EPLAN data - (.epj, .bmk, .edc)", void, string, string, string ) },
    {"buildECADgraph", PyWrap( ElectricSystem, buildECADgraph, "Build graphs", void ) },
    {"getLADVariables", PyWrap( ElectricSystem, getLADVariables, "Get LAD variables", strLadMap ) },
    {"getObjectsByName", PyWrap( ElectricSystem, getObjectsByName, "Get objects map", strObjMap ) },
    {"simECAD", PyWrap( ElectricSystem, simECAD, "Do simulation step", void ) },
    {"getRegistred", PyWrap( ElectricSystem, getRegistred, "Get components", vector<VRElectricComponentPtr>, string ) },
    {"getComponents", PyWrap( ElectricSystem, getComponents, "Get components", intECompMap ) },
    {"getComponentsByEGraphID", PyWrap( ElectricSystem, getComponentsByEGraphID, "Get components", intECompMap ) },
    {"getComponentsByPGraphID", PyWrap( ElectricSystem, getComponentsByPGraphID, "Get components", intECompMap ) },
    {"getComponentIDs", PyWrap( ElectricSystem, getComponentIDs, "Get components", strVecECompMap ) },
    {"getElectricGraph", PyWrap( ElectricSystem, getElectricGraph, "Get electric graph", GraphPtr ) },
    {"getProfinetGraph", PyWrap( ElectricSystem, getProfinetGraph, "Get profinet graph", GraphPtr ) },
    {NULL}
};

PyMethodDef VRPyElectricVisualization::methods[] = {
    {"setSystem", PyWrap( ElectricVisualization, setSystem, "Set electric system", void, VRElectricSystemPtr ) },
    {"update", PyWrap( ElectricVisualization, update, "Update visualization", void ) },
    {"updateWires", PyWrap( ElectricVisualization, updateWires, "Update visualization, only wires", void ) },
    {NULL}
};

PyMethodDef VRPyWire::methods[] = {
    {"setEntity", PyWrap( Wire, setEntity, "Set entity", void, VREntityPtr ) },
    {"getEntity", PyWrap( Wire, getEntity, "Get entity", VREntityPtr ) },
    {"getLabel", PyWrap( Wire, getLabel, "Get label", string ) },
    {"getType", PyWrap( Wire, getType, "Get type", string ) },
    {"getSource", PyWrap( Wire, getSource, "Get source address", VRElectricComponent::Address ) },
    {"getTarget", PyWrap( Wire, getTarget, "Get target address", VRElectricComponent::Address ) },
    {"getOther", PyWrap( Wire, getOther, "Get address of other component", VRElectricComponent::Address, VRElectricComponentPtr ) },
    {NULL}
};

PyMethodDef VRPyWiringSimulation::methods[] = {
    {NULL}
};

PyMethodDef VRPyElectricComponent::methods[] = {
    {"setCurrent", PyWrap( ElectricComponent, setCurrent, "Set current on port (current, port), current is '0' or '1'", void, string, string ) },
    {"setName", PyWrap( ElectricComponent, setName, "Set name", void, string ) },
    {"setMcadID", PyWrap( ElectricComponent, setMcadID, "Set mcad ID", void, string ) },
    {"setGeometry", PyWrap( ElectricComponent, setGeometry, "Set geometry", void, VRObjectPtr ) },
    {"setEntity", PyWrap( ElectricComponent, setEntity, "Set entity", void, VREntityPtr ) },
    {"setEGraphID", PyWrap( ElectricComponent, setEGraphID, "Set E graph ID", void, int ) },
    {"setPGraphID", PyWrap( ElectricComponent, setPGraphID, "Set P graph ID", void, int ) },
    {"setPortEntity", PyWrap( ElectricComponent, setPortEntity, "Set port entity", void, string, VREntityPtr ) },
    {"setPosition", PyWrap( ElectricComponent, setPosition, "Set position", void, Vec3d ) },
    {"getName", PyWrap( ElectricComponent, getName, "Get name", string ) },
    {"getWire", PyWrap( ElectricComponent, getWire, "Get wire", VRWirePtr, VRElectricComponentPtr ) },
    {"getEcadID", PyWrap( ElectricComponent, getEcadID, "Get ecad ID", string ) },
    {"getMcadID", PyWrap( ElectricComponent, getMcadID, "Get mcad ID", string ) },
    {"getGeometry", PyWrap( ElectricComponent, getGeometry, "Get geometry", VRObjectPtr ) },
    {"getEntity", PyWrap( ElectricComponent, getEntity, "Get entity", VREntityPtr ) },
    {"getConnections", PyWrap( ElectricComponent, getConnections, "Get wires", vector<VRWirePtr> ) },
    {"getPorts", PyWrap( ElectricComponent, getPorts, "Get ports names", vector<string> ) },
    {"hasAddress", PyWrap( ElectricComponent, hasAddress, "Check address", bool ) },
    {"getAddress", PyWrap( ElectricComponent, getAddress, "Get address", VRElectricComponent::Address ) },
    {"getAddressMachine", PyWrap( ElectricComponent, getAddressMachine, "Get address machine", string ) },
    {"getPortWire", PyWrap( ElectricComponent, getPortWire, "Get port wire", VRWirePtr, string ) },
    {"getPortEntity", PyWrap( ElectricComponent, getPortEntity, "Get port entity", VREntityPtr, string ) },
    {"getEGraphID", PyWrap( ElectricComponent, getEGraphID, "Get port entity", int ) },
    {"getPGraphID", PyWrap( ElectricComponent, getPGraphID, "Get port entity", int ) },
    {"getPosition", PyWrap( ElectricComponent, getPosition, "Get position", Vec3d ) },
    {"getConnection", PyWrap( ElectricComponent, getConnection, "Get connection", VRWirePtr, string ) },
    {"addPort", PyWrap( ElectricComponent, addPort, "Add port", void, string, string, string, string ) },
    {NULL}
};

PyMethodDef VRPyLADEngine::methods[] = {
    {"setElectricEngine", PyWrap( LADEngine, setElectricEngine, "Set electric system", void, VRElectricSystemPtr ) },
    {"read", PyWrap( LADEngine, read, "Read data, first path is full path to tag table 'Default tag table.xml', second path is path to modules folder '../Programmbausteine'", void, string, string ) },
    {"iterate", PyWrap( LADEngine, iterate, "Run iteration", void ) },
    {"getCompileUnits", PyWrap( LADEngine, getCompileUnits, "Return IDs of compile units", vector<string> ) },
    {"getCompileUnitWires", PyWrapOpt( LADEngine, getCompileUnitWires, "Return IDs of compile unit wires, optional pnly powered wired", "0", vector<string>, string, bool ) },
    {"getCompileUnitParts", PyWrap( LADEngine, getCompileUnitParts, "Return IDs of compile unit parts", vector<string>, string ) },
    {"getCompileUnitVariables", PyWrap( LADEngine, getCompileUnitVariables, "Return IDs of compile unit variables", vector<string>, string ) },
    {"getCompileUnitWireSignal", PyWrap( LADEngine, getCompileUnitWireSignal, "Return IDs of compile unit wire signal", int, string, string ) },
    {"getCompileUnitWireOutParts", PyWrap( LADEngine, getCompileUnitWireOutParts, "Return IDs of compile units parts that are outputs of a wire", vector<string>, string, string ) },
    {"getCompileUnitPartVariable", PyWrap( LADEngine, getCompileUnitPartVariable, "Return LAD variable from part", VRLADVariablePtr, string, string ) },
    {"getCompileUnitPartOutWires", PyWrap( LADEngine, getCompileUnitPartOutWires, "Return IDs of compile units wires that are outputs of a part", vector<string>, string, string ) },
    {"getCompileUnitPartName", PyWrap( LADEngine, getCompileUnitPartName, "Return part name", string, string, string ) },
    {"addVisual", PyWrap( LADEngine, addVisual, "Create basic visualization", VRTransformPtr ) },
    {"updateVisual", PyWrap( LADEngine, updateVisual, "Update visualization based on current state", void ) },
    {NULL}
};

PyMethodDef VRPyLADVariable::methods[] = {
    {"setName", PyWrap( LADVariable, setName, "Set name", void, string ) },
    {"setLogicalAddress", PyWrap( LADVariable, setLogicalAddress, "Set logical address", void, string ) },
    {"setDataType", PyWrap( LADVariable, setDataType, "Set data type", void, string ) },
    {"setSource", PyWrap( LADVariable, setSource, "Set source", void, string ) },
    {"setRemanence", PyWrap( LADVariable, setRemanence, "Set remanence", void, string ) },
    {"setValue", PyWrap( LADVariable, setValue, "Set value", void, string ) },
    {"setStartValue", PyWrap( LADVariable, setStartValue, "Set start value", void, string ) },
    {"getName", PyWrap( LADVariable, getName, "Get name", string ) },
    {"getLogicalAddress", PyWrap( LADVariable, getLogicalAddress, "Get logical address", string ) },
    {"getDataType", PyWrap( LADVariable, getDataType, "Get data type", string ) },
    {"getSource", PyWrap( LADVariable, getSource, "Get source", string ) },
    {"getRemanence", PyWrap( LADVariable, getRemanence, "Get remanence", string ) },
    {"getValue", PyWrap( LADVariable, getValue, "Get value", string ) },
    {"getStartValue", PyWrap( LADVariable, getStartValue, "Get start value", string ) },
    {NULL}
};

PyMethodDef VRPySCLScript::methods[] = {
    {"readSCL", PyWrap( SCLScript, readSCL, "read a scl script", void, string ) },
    {"convert", PyWrap( SCLScript, convert, "convert the scl script to python", void ) },
    {"getScl", PyWrap( SCLScript, getScl, "get the scl script", string ) },
    {"getPython", PyWrap( SCLScript, getPython, "get the python script", string ) },
    {NULL}
};

PyMethodDef VRPySCLEngine::methods[] = {
    {"setElectricEngine", PyWrap( SCLEngine, setElectricEngine, "set the electric system", void, VRElectricSystemPtr ) },
    {"readSCL", PyWrap( SCLEngine, readSCL, "read an scl script and name it, returns the scl script", VRSCLScriptPtr, string, string ) },
    {"getScript", PyWrap( SCLEngine, getScript, "return scl script by name", VRSCLScriptPtr, string ) },
    {"iterate", PyWrap( SCLEngine, iterate, "run tick", void ) },
    {NULL}
};

PyMethodDef VRPyNumberingEngine::methods[] = {
    {"set", PyWrapOpt( NumberingEngine, set, "Set number: ID, pos, number, decimal places, groupID", "2|0", void, int, Vec3d, float, int, int ) },
    {"clear", PyWrap( NumberingEngine, clear, "Clear numbers", void ) },
    {NULL}
};

PyMethodDef VRPyRobotArm::methods[] = {
    {"showAnalytics", PyWrap(RobotArm, showAnalytics, "Shows a visualization of the analytic model", void, bool ) },
    {"setParts", PyWrap(RobotArm, setParts, "Set robot parts\n kuka: [upperBase, beam, elbow, beam, wrist {, tool, finger1, finger2 } ]", void, vector<VRTransformPtr> ) },
    {"genKinematics", PyWrap(RobotArm, genKinematics, "Gen kinematics: base > upperBase > beam > elbow > beam > wrist", VRTransformPtr ) },
    {"setAngleOffsets", PyWrap(RobotArm, setAngleOffsets, "Set angle offset for each part", void, vector<float> ) },
    {"setAngleDirections", PyWrap(RobotArm, setAngleDirections, "Set angles rotation direction - setAngleDirections([1/-1])", void, vector<int> ) },
    {"setAxis", PyWrap(RobotArm, setAxis, "Set rotation axis for each part - setAxis([int a])\n a: 0 = 'x', 1 = 'y', 2 = 'z'", void, vector<int> ) },
    {"setLengths", PyWrap(RobotArm, setLengths, "Set kinematic lengths between joints - setLengths([base_height, upper_arm length, forearm length, grab position])", void, vector<float> ) },
    {"setAxisOffsets", PyWrap(RobotArm, setAxisOffsets, "Set offsets of rotation axis - setAxisOffsets([shoulder_offset])", void, vector<float> ) },
    {"setSpeed", PyWrap(RobotArm, setSpeed, "Set path follow animation speed", void, float ) },
    {"setMaxSpeed", PyWrap(RobotArm, setMaxSpeed, "Set max angular speed", void, float ) },
    {"canReach", PyWrapOpt(RobotArm, canReach, "Check if the end effector can reach a certain pose, optionally in local robot coords, default in world coords - canReach(pose | local)", "0", bool, PosePtr, bool ) },
    {"moveTo", PyWrapOpt(RobotArm, moveTo, "Move the end effector to a certain pose, optionally in local robot coords, default in world coords - moveTo(pose | local)", "0", void, PosePtr, bool ) },
    {"setGrab", PyWrap(RobotArm, setGrab, "Set grab state - setGrab(float d)\n d: 0 is closed, 1 is open", void, float ) },
    {"toggleGrab", PyWrap(RobotArm, toggleGrab, "Toggle the grab - toggleGrab()", void ) },
    {"grab", PyWrap(RobotArm, grab, "Grab object", void, VRTransformPtr ) },
    {"drop", PyWrap(RobotArm, drop, "Drop held object", VRTransformPtr ) },
    {"stop", PyWrap(RobotArm, stop, "Stop and reset everything", void ) },
    {"pause", PyWrap(RobotArm, pause, "Pause movement", void ) },
    {"move", PyWrap(RobotArm, move, "Resume movement", void ) },
    {"setAngles", PyWrapOpt(RobotArm, setAngles, "Set joint angles - setAngles( angles, force )", "0", void, vector<float>, bool ) },
    {"getAngles", PyWrap(RobotArm, getAngles, "Get current joint angles", vector<float> ) },
    {"getTargetAngles", PyWrap(RobotArm, getTargetAngles, "Get target joint angles", vector<float> ) },
    //{"getForwardKinematics", PyWrap(RobotArm, getForwardKinematics, "Get end effector pose from angles - p,d,u getForwardKinematics( angles )") },
    //{"getBackwardKinematics", PyWrap(RobotArm, getBackwardKinematics, "Get angles from end effector pose - angles getBackwardKinematics( p,d,u )") },
    {"setPath", PyWrapOpt(RobotArm, setPath, "Set robot path(s) - second path is optinal and overrides orientation", "0", void, PathPtr, PathPtr ) },
    {"getPath", PyWrap(RobotArm, getPath, "Get robot path", PathPtr ) },
    {"getOrientationPath", PyWrap(RobotArm, getOrientationPath, "Get robot orientation path", PathPtr ) },
    {"getParts", PyWrap(RobotArm, getParts, "Get robot parts", vector<VRTransformPtr> ) },
    {"moveOnPath", PyWrapOpt(RobotArm, moveOnPath, "Move robot on internal path - moveOnPath(t0, t1, doLoop=0, durationMultiplier=1, local=0)", "0|1|0", void, float, float, bool, float, bool) },
    {"isMoving", PyWrap(RobotArm, isMoving, "Get animation status", bool) },
    {"setEventCallback", PyWrap(RobotArm, setEventCallback, "Set callback for move and stop events", void, VRMessageCbPtr) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRocketExhaust::methods[] = {
    {"set", PyWrap( RocketExhaust, set, "Set exhaust amount, from 0.0 to 1.0", void, float ) },
    {NULL}
};

typedef map<double, string> mapStrDouble;

PyMethodDef VRPySpaceMission::methods[] = {
    {"setParameters", PyWrap( SpaceMission, setParameters, "Set name, start, stop", void, string, double, double ) },
    {"addWayPoint", PyWrap( SpaceMission, addWayPoint, "Add way point, name, time", void, string, double ) },
    {"getName", PyWrap( SpaceMission, getName, "Return mission name", string ) },
    {"getWayPoints", PyWrap( SpaceMission, getWayPoints, "Return waypoints", mapStrDouble ) },
    {NULL}
};
