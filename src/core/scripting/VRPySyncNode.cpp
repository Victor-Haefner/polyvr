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
    {"addRemote", PyWrap(SyncNode, addRemote, "adds a SyncRemote, address and port", void, string, int) },
    {"broadcast", PyWrap(SyncNode, broadcast, "broadcasts a message to all SyncRemotes", void, string) },
    {"update", PyWrap(SyncNode, update, "update", void) },
    {"analyseSubGraph", PyWrap(SyncNode, analyseSubGraph, "Print subgraph to console", void) },
    {"getOwnedObjects", PyWrap(SyncNode, getOwnedObjects, "getOwnedObjects", vector<string>, string) },
    {"requestOwnership", PyWrap(SyncNode, requestOwnership, "requestOwnership( objectName )", void, string) },
    {"addOwnedObject", PyWrap(SyncNode, addOwnedObject, "addOwnedObject ( objectName )", void, string) },
    {"setDoWrapping", PyWrap(SyncNode, setDoWrapping, "Set if doing OSG wrapping", void, bool) },
    {"setAvatarBeacons", PyWrap(SyncNode, setAvatarBeacons, "Set own avatar beacons, usually camera, mouse beacon, mouse beacon", void, VRTransformPtr, VRTransformPtr, VRTransformPtr) },
    {"addRemoteAvatar", PyWrap(SyncNode, addRemoteAvatar, "Add avatar components, a geometry for the head, another for the hand, and a transform attached to the hand for DnD", void, string, string, VRTransformPtr, VRTransformPtr, VRTransformPtr) },
    {"getConnectionLink", PyWrap(SyncNode, getConnectionLink, "Get Connection Link", string) },
    {"getConnectionStatus", PyWrap(SyncNode, getConnectionStatus, "Set syncNode callback", string) },
    {"getRemotes", PyWrap(SyncNode, getRemotes, "Get list with uris of remotes", vector<string>) },
    {"setTCPClient", PyWrap(SyncNode, setTCPClient, "Set TCP client, usually from an ICE session", string, VRNetworkClientPtr) },
    {"addTCPClient", PyWrap(SyncNode, addTCPClient, "Add TCP client, usually from an ICE session", string, VRNetworkClientPtr) },
    {NULL}  /* Sentinel */
};

//PyMethodDef VRPySyncRemote::methods[] = {
//    {NULL}  /* Sentinel */
//};

//PyMethodDef FieldContainerFactoryBase::methods[] = {
//    {"getContainerFactory", PyWrap(FieldContainerFactory, FieldContainerFactory::the(), "FieldContainerFactory", FieldContainerFactory*) },
//    {NULL}  /* Sentinel */
//};

