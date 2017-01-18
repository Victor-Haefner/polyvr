#include "VRPyTree.h"
#include "core/scripting/VRPyMaterial.h"
#include "core/scripting/VRPyImage.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Terrain, New_VRObjects_ptr);
simpleVRPyType(Tree, New_VRObjects_unnamed_ptr);

PyMethodDef VRPyTree::methods[] = {
    {"setup", (PyCFunction)VRPyTree::setup, METH_VARARGS, "Set the tree parameters - setup( int , int, int, flt, flt, flt, flt, flt, flt, flt, flt ) " },
    {"addLeafs", (PyCFunction)VRPyTree::addLeafs, METH_VARARGS, "Add a leaf layer - addLeafs( int lvl, flt scale, flt aspect )" },
    {"setLeafMaterial", (PyCFunction)VRPyTree::setLeafMaterial, METH_VARARGS, "Set leaf material - setLeafMaterial( mat )" },
    {NULL}  /* Sentinel */
};

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
    if (! PyArg_ParseTuple(args, "ii", &lvl, &amount)) return NULL;
    self->objPtr->addLeafs( lvl, amount );
    Py_RETURN_TRUE;
}

PyObject* VRPyTree::setLeafMaterial(VRPyTree* self, PyObject* args) {
    VRPyMaterial* m;
    if (! PyArg_ParseTuple(args, "O", &m)) return NULL;
    self->objPtr->setLeafMaterial( m->objPtr );
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


