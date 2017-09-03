#include "VRPyNature.h"
#include "core/scripting/VRPyMaterial.h"
#include "core/scripting/VRPyImage.h"
#include "core/scripting/VRPyPolygon.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

template<> PyObject* VRPyTypeCaster::cast(const VRNaturePtr& e) { return VRPyNature::fromSharedPtr(e); }
template<> bool toValue(PyObject* o, VRNaturePtr& p) { if (!VRPyNature::check(o)) return 0; p = ((VRPyNature*)o)->objPtr; return 1; }
template<> PyObject* VRPyTypeCaster::cast(const VRTreePtr& e) { return VRPyTree::fromSharedPtr(e); }
template<> bool toValue(PyObject* o, VRTreePtr& p) { if (!VRPyTree::check(o)) return 0; p = ((VRPyTree*)o)->objPtr; return 1; }

simpleVRPyType(Tree, New_VRObjects_ptr);
simpleVRPyType(Nature, New_VRObjects_ptr);

PyMethodDef VRPyTree::methods[] = {
    {"setup", (PyCFunction)VRPyTree::setup, METH_VARARGS, "Set the tree parameters - setup( int , int, int, flt, flt, flt, flt, flt, flt, flt, flt ) " },
    {"addBranching", (PyCFunction)VRPyTree::addBranching, METH_VARARGS, "Set the tree parameters - addBranching( int nodes, int branching, flt, flt, flt, flt, flt, flt, flt, flt ) " },
    {"grow", (PyCFunction)VRPyTree::grow, METH_VARARGS, "Set the tree parameters - grow( flt seed ) " },
    {"addLeafs", (PyCFunction)VRPyTree::addLeafs, METH_VARARGS, "Add a leaf layer - addLeafs( int lvl, int amount, flt size )" },
    {"setLeafMaterial", (PyCFunction)VRPyTree::setLeafMaterial, METH_VARARGS, "Set leaf material - setLeafMaterial( mat )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyTree::addBranching(VRPyTree* self, PyObject* args) {
    int a,b;
    float d,e,f,g,h,i,j,k;
    if (! PyArg_ParseTuple(args, "iiffffffff", &a, &b, &d, &e, &f, &g, &h, &i, &j, &k)) return NULL;

    self->objPtr->addBranching( a,b, d,e,f,g, h,i,j,k );
    Py_RETURN_TRUE;
}

PyObject* VRPyTree::grow(VRPyTree* self, PyObject* args) {
    float s;
    if (! PyArg_ParseTuple(args, "f", &s)) return NULL;

    self->objPtr->grow(s);
    Py_RETURN_TRUE;
}

PyObject* VRPyTree::setup(VRPyTree* self, PyObject* args) {
    int a,b,c;
    float d,e,f,g,h,i,j,k;
    if (! PyArg_ParseTuple(args, "iiiffffffff", &a, &b, &c, &d, &e, &f, &g, &h, &i, &j, &k)) return NULL;

    self->objPtr->setup( a,b,c, d,e,f,g, h,i,j,k );
    Py_RETURN_TRUE;
}

PyObject* VRPyTree::addLeafs(VRPyTree* self, PyObject* args) {
    int lvl = 1;
    int amount = 1;
    float size = 0.03;
    if (! PyArg_ParseTuple(args, "ii|f", &lvl, &amount, &size)) return NULL;
    self->objPtr->addLeafs( lvl, amount, size );
    Py_RETURN_TRUE;
}

PyObject* VRPyTree::setLeafMaterial(VRPyTree* self, PyObject* args) {
    VRPyMaterial* m;
    if (! PyArg_ParseTuple(args, "O", &m)) return NULL;
    self->objPtr->setLeafMaterial( m->objPtr );
    Py_RETURN_TRUE;
}


PyMethodDef VRPyNature::methods[] = {
    {"addTree", PyWrapOpt(Nature, addTree, "Add a copy of the passed tree to the woods and return the copy", "0|1", VRTreePtr, VRTreePtr, bool, bool ) },
    {"addGrassPatch", PyWrapOpt(Nature, addGrassPatch, "Add a grass patch from polygon", "0|0|0", void, VRPolygonPtr, bool, bool) },
    {"computeLODs", PyWrap(Nature, computeLODs, "Compute LODs - computeLODs() ", void ) },
    {"addCollisionModels", PyWrap(Nature, addCollisionModels, "Add collision box to trees and bushes - addCollisionModels() ", void ) },
    {"clear", PyWrap(Nature, clear, "Clear woods", void ) },
    {"getTree", PyWrap(Nature, getTree, "Get a tree by id", VRTreePtr, int ) },
    {"removeTree", PyWrap(Nature, removeTree, "Remove a tree by id", void, int ) },
    {"simpleInit", PyWrap(Nature, simpleInit, "Add a few random tree and bush types", void, int, int) },
    {NULL}  /* Sentinel */
};



