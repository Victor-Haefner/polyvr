#include "VRPySyncNode.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(SyncNode, New_VRObjects_ptr);
//simpleVRPyType(SyncRemote, New_VRObjects_ptr);

PyMethodDef VRPySyncNode::methods[] = {
    {"printChangeList", PyWrap(SyncNode, printChangeList, "void printChangeList()", void) },
    {NULL}  /* Sentinel */
};

//PyMethodDef VRPySyncRemote::methods[] = {
//    {NULL}  /* Sentinel */
//};


