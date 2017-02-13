#include "VRPyTree.h"
#include "core/scripting/VRPyMaterial.h"
#include "core/scripting/VRPyImage.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Terrain, New_VRObjects_ptr);
simpleVRPyType(Tree, New_VRObjects_unnamed_ptr);
simpleVRPyType(Woods, New_VRObjects_unnamed_ptr);

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

PyMethodDef VRPyWoods::methods[] = {
    {"addTree", (PyCFunction)VRPyWoods::addTree, METH_VARARGS, "Add a tree - addTree( tree ) " },
    {"computeLODs", (PyCFunction)VRPyWoods::computeLODs, METH_NOARGS, "Compute LODs - computeLODs() " },
    {NULL}  /* Sentinel */
};

PyObject* VRPyWoods::addTree(VRPyWoods* self, PyObject* args) {
    VRPyTree* o = 0;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    self->objPtr->addTree( o->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyWoods::computeLODs(VRPyWoods* self) {
    self->objPtr->computeLODs();
    Py_RETURN_TRUE;
}

PyMethodDef VRPyTerrain::methods[] = {
    {"setParameters", (PyCFunction)VRPyTerrain::setParameters, METH_VARARGS, "Set the terrain parameters - setParameters( [x,y] size, float resolution ) " },
    {"setMap", (PyCFunction)VRPyTerrain::setMap, METH_VARARGS, "Set height map - setMap( texture )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyTerrain::setParameters(VRPyTerrain* self, PyObject* args) {
    PyObject* o = 0;
    float s;
    if (! PyArg_ParseTuple(args, "Of", &o, &s)) return NULL;
    self->objPtr->setParameters( parseVec2fList(o), s );
    Py_RETURN_TRUE;
}

PyObject* VRPyTerrain::setMap(VRPyTerrain* self, PyObject* args) {
    VRPyImage* t = 0;
    if (! PyArg_ParseTuple(args, "O", &t)) return NULL;
    self->objPtr->setMap( t->objPtr );
    Py_RETURN_TRUE;
}


