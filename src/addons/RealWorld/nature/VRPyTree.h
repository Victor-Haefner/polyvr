#ifndef VRPYTREE_H_INCLUDED
#define VRPYTREE_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRTree.h"

struct VRPyTree : VRPyBaseT<OSG::VRTree> {
    static PyMethodDef methods[];

    static PyObject* setup(VRPyTree* self, PyObject* args);
};

#endif // VRPYTREE_H_INCLUDED
