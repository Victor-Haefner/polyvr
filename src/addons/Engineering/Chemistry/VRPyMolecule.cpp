#include "VRPyMolecule.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"


template<> PyTypeObject VRPyBaseT<OSG::VRMolecule>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Molecule",             /*tp_name*/
    sizeof(VRPyMolecule),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Molecule binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyMolecule::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_VRObjects,                 /* tp_new */
};

PyMethodDef VRPyMolecule::methods[] = {
    {"set", (PyCFunction)VRPyMolecule::set, METH_VARARGS, "Set the molecule from string - set('CH4')" },
    {"setRandom", (PyCFunction)VRPyMolecule::setRandom, METH_VARARGS, "Set a random molecule - setRandom(123)" },
    {"showLabels", (PyCFunction)VRPyMolecule::showLabels, METH_VARARGS, "Display the ID of each atom - showLabels(True)" },
    {"showCoords", (PyCFunction)VRPyMolecule::showCoords, METH_VARARGS, "Display the coordinate system of each atom - showCoords(True)" },
    {"substitute", (PyCFunction)VRPyMolecule::substitute, METH_VARARGS, "Substitute an atom of both molecules to append the second to this - substitute(int aID, mol b, int bID)" },
    {"rotateBond", (PyCFunction)VRPyMolecule::rotateBond, METH_VARARGS, "Rotate the bon between atom a and b - rotateBond(int aID, int bID, float a)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMolecule::set(VRPyMolecule* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMolecule::set - Object is invalid"); return NULL; }
    self->obj->set( parseString(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyMolecule::setRandom(VRPyMolecule* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMolecule::setRandom - Object is invalid"); return NULL; }
    self->obj->setRandom( parseInt(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyMolecule::showLabels(VRPyMolecule* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMolecule::showLabels - Object is invalid"); return NULL; }
    self->obj->showLabels( parseBool(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyMolecule::showCoords(VRPyMolecule* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMolecule::showCoords - Object is invalid"); return NULL; }
    self->obj->showCoords( parseBool(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyMolecule::rotateBond(VRPyMolecule* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMolecule::rotateBond - Object is invalid"); return NULL; }
    int a, b; float f;
    if (! PyArg_ParseTuple(args, "iif", &a, &b, &f)) return NULL;
    self->obj->rotateBond( a, b, f );
    Py_RETURN_TRUE;
}

PyObject* VRPyMolecule::substitute(VRPyMolecule* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMolecule::substitute - Object is invalid"); return NULL; }

    VRPyMolecule* mB; int a, b;
    if (! PyArg_ParseTuple(args, "iOi", &a, &mB, &b)) return NULL;
    if ((PyObject*)mB == Py_None) { PyErr_SetString(err, "VRPyMolecule::substitute - molecule is invalid"); return NULL; }

    self->obj->substitute( a, mB->obj, b );
    Py_RETURN_TRUE;
}
