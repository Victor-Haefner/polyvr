#ifndef VRPYTREE_H_INCLUDED
#define VRPYTREE_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRTree.h"
#include "VRNature.h"

struct VRPyTree : VRPyBaseT<OSG::VRTree> {
    static PyMethodDef methods[];
};

struct VRPyNature : VRPyBaseT<OSG::VRNature> {
    static PyMethodDef methods[];
};

#endif // VRPYTREE_H_INCLUDED
