#include "VRPyMechanism.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyTypeCaster.h"


template<> PyTypeObject VRPyBaseT<OSG::VRMechanism>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Mechanism",             /*tp_name*/
    sizeof(VRPyMechanism),             /*tp_basicsize*/
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
    "Mechanism binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyMechanism::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New,                 /* tp_new */
};

PyMethodDef VRPyMechanism::methods[] = {
    {"add", (PyCFunction)VRPyMechanism::add, METH_VARARGS, "Add part to mechanism - add(P)" },
    {"update", (PyCFunction)VRPyMechanism::update, METH_NOARGS, "Update mechanism simulation" },
    {"clear", (PyCFunction)VRPyMechanism::clear, METH_NOARGS, "Clear mechanism parts" },
    {"addChain", (PyCFunction)VRPyMechanism::addChain, METH_VARARGS, "Add chain - addChain(float width, [G1, G2, G3, ...])" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMechanism::add(VRPyMechanism* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMechanism::add - Object is invalid"); return NULL; }

    VRPyGeometry* geo;
    if (! PyArg_ParseTuple(args, "O", &geo)) return NULL;

    self->obj->add(geo->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyMechanism::update(VRPyMechanism* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMechanism::update - Object is invalid"); return NULL; }
    self->obj->update();
    Py_RETURN_TRUE;
}

PyObject* VRPyMechanism::clear(VRPyMechanism* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMechanism::clear - Object is invalid"); return NULL; }
    self->obj->clear();
    Py_RETURN_TRUE;
}

PyObject* VRPyMechanism::addChain(VRPyMechanism* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMechanism::addChain - Object is invalid"); return NULL; }
    float w; PyObject *l, *dirs;
    if (! PyArg_ParseTuple(args, "fOO", &w, &l, &dirs)) return NULL;
    vector<PyObject*> objs = pyListToVector(l);
    vector<OSG::VRGeometry*> geos;
    for (auto o : objs) {
        VRPyGeometry* g = (VRPyGeometry*)o;
        geos.push_back( g->obj );
    }
    return VRPyTypeCaster::cast( self->obj->addChain(w, geos, PyString_AsString(dirs) ) );
}
