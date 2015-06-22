#include "VRPyLod.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRLod>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Lod",             /*tp_name*/
    sizeof(VRPyLod),             /*tp_basicsize*/
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
    "VRLod binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyLod::methods,             /* tp_methods */
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

PyMethodDef VRPyLod::methods[] = {
	{"setCenter", (PyCFunction)VRPyLod::setCenter, METH_VARARGS, "Set the center from which the LOD distance is calculated" },
	{"setDistance", (PyCFunction)VRPyLod::setDistance, METH_VARARGS, "Set the distance at which the specified LOD stage should be shown" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyLod::setCenter(VRPyLod* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyLod::setCenter, Object is invalid"); return NULL; }
    OSG::Vec3f t = parseVec3f(args);

    OSG::VRLod* e = (OSG::VRLod*) self->obj;
    e->setCenter(t);
    Py_RETURN_TRUE;
}

PyObject* VRPyLod::setDistance(VRPyLod* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyLod::setCenter, Object is invalid"); return NULL; }
	int childIndex;
	float distance;
    if (!PyArg_ParseTuple(args, "if", &childIndex, &distance)) return NULL;
    OSG::VRLod* e = (OSG::VRLod*) self->obj;
    e->setDistance(childIndex, distance);
    Py_RETURN_TRUE;
}
