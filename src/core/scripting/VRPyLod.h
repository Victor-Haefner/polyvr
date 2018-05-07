#ifndef VRPYLOD_H
#define VRPYLOD_H

#include "VRPyBase.h"
#include "core/objects/VRLod.h"
#include "core/objects/VRLodTree.h"

struct VRPyLod : VRPyBaseT<OSG::VRLod> {
    static PyMethodDef methods[];
};

struct VRPyLodTree : VRPyBaseT<OSG::VRLodTree> {
    static PyMethodDef methods[];
};

struct VRPyLodLeaf : VRPyBaseT<OSG::VRLodLeaf> {
    static PyMethodDef methods[];
};

#endif // VRPYLOD_H
