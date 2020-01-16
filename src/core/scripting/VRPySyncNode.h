#ifndef VRPySyncNode_H_INCLUDED
#define VRPySyncNode_H_INCLUDED

#include "VRPyObject.h"
#include "core/objects/VRSyncNode.h"

struct VRPySyncNode : VRPyBaseT<OSG::VRSyncNode> {
    static PyMethodDef methods[];
};

struct VRPySyncRemote : VRPyBaseT<OSG::VRSyncRemote> {
    static PyMethodDef methods[];
};
#endif // VRPySyncNode_H_INCLUDED
