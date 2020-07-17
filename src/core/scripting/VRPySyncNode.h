#ifndef VRPySyncNode_H_INCLUDED
#define VRPySyncNode_H_INCLUDED

#include "VRPyObject.h"
#include "core/objects/sync/VRSyncNode.h"

struct VRPySyncNode : VRPyBaseT<OSG::VRSyncNode> {
    static PyMethodDef methods[];
};

//struct VRPySyncRemote : VRPyBaseT<OSG::VRSyncRemote> {
//    static PyMethodDef methods[];
//};
//struct VRPyFieldContainerFactory : VRPyBaseT<OSG::FieldContainerFactory> {
//    static PyMethodDef methods[];
//};
#endif // VRPySyncNode_H_INCLUDED
