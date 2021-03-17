#include "VRPyOPCUA.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(OPCUA, New_ptr);
simpleVRPyType(OPCUANode, 0);

PyMethodDef VRPyOPCUANode::methods[] = {
    {"name", PyWrap( OPCUANode, name, "Get node name", string ) },
    {"value", PyWrap( OPCUANode, value, "Get node value", string ) },
    {"type", PyWrap( OPCUANode, type, "Get node type", string ) },
    {"ID", PyWrap( OPCUANode, ID, "Get node ID", string ) },
    {"getChildren", PyWrap( OPCUANode, getChildren, "Get node children", vector<VROPCUANodePtr> ) },
    {"set", PyWrapOpt( OPCUANode, set, "Set node value", "1", void, string, bool ) },
    {"setVector", PyWrap( OPCUANode, setVector, "Set node array value", void, vector<string> ) },
    {"getChild", PyWrap( OPCUANode, getChild, "Get child i", VROPCUANodePtr, int ) },
    {"getChildByName", PyWrap( OPCUANode, getChildByName, "Get child by name", VROPCUANodePtr, string ) },
    {"getChildAtPath", PyWrap( OPCUANode, getChildAtPath, "Get child at path, the path are the names connected with '.'", VROPCUANodePtr, string ) },
    {"subscribe", PyWrap( OPCUANode, subscribe, "Subscribe to OPCUA server", void, VROPCUANodeCbPtr ) },
    {"isSubscribed", PyWrap( OPCUANode, isSubscribed, "Returns if subscribed to server", bool ) },
    {"test", PyWrap( OPCUANode, test, "A test ..duh", string ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyOPCUA::methods[] = {
    {"connect", PyWrap( OPCUA, connect, "Connect to server", VROPCUANodePtr, string ) },
    {"setupTestServer", PyWrap( OPCUA, setupTestServer, "Start test server", void ) },
    {NULL}  /* Sentinel */
};
