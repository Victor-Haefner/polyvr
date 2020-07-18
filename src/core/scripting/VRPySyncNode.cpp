#include "VRPySyncNode.h"
#include "VRPyBaseT.h"
//#include "core/math/pose.h"

using namespace OSG;

simpleVRPyType(SyncNode, New_VRObjects_ptr);
//simpleVRPyType(SyncRemote, New_VRObjects_ptr);
//simpleVRPyType(FieldContainerFactory, New_VRObjects_ptr);

PyMethodDef VRPySyncNode::methods[] = {
    //{"printCurrentChangeList", PyWrap(SyncNode, printCurrentChangeList, "prints the current ChangeList", void) },
    {"startInterface", PyWrap(SyncNode, startInterface, "updates this SyncNode", void, int) },
    {"addRemote", PyWrap(SyncNode, addRemote, "adds a SyncRemote", void, string, int, string) },
    {"broadcast", PyWrap(SyncNode, broadcast, "broadcasts a message to all SyncRemotes", void, string) },
    {"update", PyWrap(SyncNode, update, "update", void) },
    {"analyseSubGraph", PyWrap(SyncNode, analyseSubGraph, "Print subgraph to console", void) },
    {"getRemoteCamPose", PyWrap(SyncNode, getRemoteCamPose, "getRemoteCamPose", PosePtr, string) },
    {"getRemoteMousePose", PyWrap(SyncNode, getRemoteMousePose, "getRemoteMousePose", PosePtr, string) },
    {"getOwnedObjects", PyWrap(SyncNode, getOwnedObjects, "getOwnedObjects", vector<string>, string) },
    {"requestOwnership", PyWrap(SyncNode, requestOwnership, "requestOwnership( objectName )", void, string) },
    {"addOwnedObject", PyWrap(SyncNode, addOwnedObject, "addOwnedObject ( objectName )", void, string) },
    {NULL}  /* Sentinel */
};

//PyMethodDef VRPySyncRemote::methods[] = {
//    {NULL}  /* Sentinel */
//};

//PyMethodDef FieldContainerFactoryBase::methods[] = {
//    {"getContainerFactory", PyWrap(FieldContainerFactory, FieldContainerFactory::the(), "FieldContainerFactory", FieldContainerFactory*) },
//    {NULL}  /* Sentinel */
//};

