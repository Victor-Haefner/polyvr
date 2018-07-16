#include "VRPyOPCUA.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(OPCUA, New_ptr);
simpleVRPyType(OPCUANode, 0);

PyMethodDef VRPyOPCUANode::methods[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyOPCUA::methods[] = {
    {"connect", PyWrap( OPCUA, connect, "Connect to server", void, string ) },
    {"setupTestServer", PyWrap( OPCUA, setupTestServer, "Start test server", void ) },
    {NULL}  /* Sentinel */
};
