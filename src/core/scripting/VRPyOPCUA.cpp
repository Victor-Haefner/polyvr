#include "VRPyOPCUA.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(OPCUA, New_ptr);
simpleVRPyType(OPCUANode, 0);

PyMethodDef VRPyOPCUANode::methods[] = {
    {"name", PyWrap( OPCUANode, name, "Get node name", string ) },
    {"value", PyWrap( OPCUANode, value, "Get node value", string ) },
    {"type", PyWrap( OPCUANode, type, "Get node type", string ) },
    {"ID", PyWrap( OPCUANode, ID, "Get node ID", int ) },
    {"getChildren", PyWrap( OPCUANode, getChildren, "Get node children", vector<VROPCUANodePtr> ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyOPCUA::methods[] = {
    {"connect", PyWrap( OPCUA, connect, "Connect to server", VROPCUANodePtr, string ) },
    {"setupTestServer", PyWrap( OPCUA, setupTestServer, "Start test server", void ) },
    {NULL}  /* Sentinel */
};
