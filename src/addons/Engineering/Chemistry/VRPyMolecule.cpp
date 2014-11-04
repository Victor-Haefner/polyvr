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
