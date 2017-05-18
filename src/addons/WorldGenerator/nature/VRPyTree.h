#ifndef VRPYTREE_H_INCLUDED
#define VRPYTREE_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRTree.h"
#include "VRTerrain.h"
#include "VRWoods.h"

struct VRPyTree : VRPyBaseT<OSG::VRTree> {
    static PyMethodDef methods[];

    static PyObject* setup(VRPyTree* self, PyObject* args);
    static PyObject* addBranching(VRPyTree* self, PyObject* args);
    static PyObject* grow(VRPyTree* self, PyObject* args);
    static PyObject* addLeafs(VRPyTree* self, PyObject* args);
    static PyObject* setLeafMaterial(VRPyTree* self, PyObject* args);
};

struct VRPyTerrain : VRPyBaseT<OSG::VRTerrain> {
    static PyMethodDef methods[];

    static PyObject* setParameters(VRPyTerrain* self, PyObject* args);
    static PyObject* setMap(VRPyTerrain* self, PyObject* args);
};

struct VRPyWoods : VRPyBaseT<OSG::VRWoods> {
    static PyMethodDef methods[];

    static PyObject* addTree(VRPyWoods* self, PyObject* args);
    static PyObject* computeLODs(VRPyWoods* self);
    static PyObject* clear(VRPyWoods* self);
    static PyObject* getTree(VRPyWoods* self, PyObject* args);
    static PyObject* removeTree(VRPyWoods* self, PyObject* args);
};

#endif // VRPYTREE_H_INCLUDED
