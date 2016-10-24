#include "VRPyTree.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Tree, New_VRObjects_unnamed_ptr);

PyMethodDef VRPyTree::methods[] = {
    {"setup", (PyCFunction)VRPyTree::setup, METH_VARARGS, "Set the tree variables - setup( int , int, int, flt, flt, flt, flt, flt, flt, flt, flt ) " },
    {"addLeafs", (PyCFunction)VRPyTree::addLeafs, METH_VARARGS, "Add a leaf layer - addLeafs( str texture, int lvl, flt scale, flt aspect )" },
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
    const char* tex = 0;
    int lvl = 1;
    float s, a;
    if (! PyArg_ParseTuple(args, "siff", (char*)&tex, &lvl, &s, &a)) return NULL;

    self->objPtr->addLeafs( tex, lvl, s, a );
    Py_RETURN_TRUE;
}
