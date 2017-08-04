#include "VRPyNature.h"
#include "core/scripting/VRPyMaterial.h"
#include "core/scripting/VRPyImage.h"
#include "core/scripting/VRPyPolygon.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

template<> PyObject* VRPyTypeCaster::cast(const VRNaturePtr& e) { return VRPyNature::fromSharedPtr(e); }
template<> bool toValue(PyObject* o, VRNaturePtr& p) { if (!VRPyNature::check(o)) return 0; p = ((VRPyNature*)o)->objPtr; return 1; }

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
    {"addTree", (PyCFunction)VRPyNature::addTree, METH_VARARGS, "Add a copy of the passed tree to the woods and return the copy - tree addTree( tree | bool updateLODs ) " },
    {"addGrassPatch", PyWrapOpt(Nature, addGrassPatch, "Add a grass patch from polygon", "0|0|0", void, VRPolygonPtr, bool, bool, bool) },
    {"computeLODs", (PyCFunction)VRPyNature::computeLODs, METH_NOARGS, "Compute LODs - computeLODs() " },
    {"addCollisionModels", (PyCFunction)VRPyNature::addCollisionModels, METH_NOARGS, "Add collision box to trees and bushes - addCollisionModels() " },
    {"clear", (PyCFunction)VRPyNature::clear, METH_NOARGS, "Clear woods - clear() " },
    {"getTree", (PyCFunction)VRPyNature::getTree, METH_VARARGS, "Get a tree by id - getTree( int ) " },
    {"removeTree", (PyCFunction)VRPyNature::removeTree, METH_VARARGS, "Remove a tree by id - removeTree( int ) " },
    {NULL}  /* Sentinel */
};

PyObject* VRPyNature::addGrassPatch(VRPyNature* self, PyObject* args) {
    VRPyPolygon* o = 0;
    int u = 0;
    if (! PyArg_ParseTuple(args, "O|i", &o, &u)) return NULL;
    //return VRPyTransform::fromSharedPtr( self->objPtr->addGrassPatch( o->objPtr, u ) );
    self->objPtr->addGrassPatch( o->objPtr, u );
    Py_RETURN_TRUE;
}

PyObject* VRPyNature::addTree(VRPyNature* self, PyObject* args) {
    VRPyTree* o = 0;
    int u = 0;
    if (! PyArg_ParseTuple(args, "O|i", &o, &u)) return NULL;
    return VRPyTree::fromSharedPtr( self->objPtr->addTree( o->objPtr, u ) );
}

PyObject* VRPyNature::removeTree(VRPyNature* self, PyObject* args) {
    int i = 0;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    self->objPtr->remTree(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyNature::getTree(VRPyNature* self, PyObject* args) {
    int i = 0;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    return VRPyTree::fromSharedPtr( self->objPtr->getTree(i) );
}

PyObject* VRPyNature::clear(VRPyNature* self) {
    self->objPtr->clear();
    Py_RETURN_TRUE;
}

PyObject* VRPyNature::addCollisionModels(VRPyNature* self) {
    self->objPtr->addCollisionModels();
    Py_RETURN_TRUE;
}

PyObject* VRPyNature::computeLODs(VRPyNature* self) {
    self->objPtr->computeLODs();
    Py_RETURN_TRUE;
}



