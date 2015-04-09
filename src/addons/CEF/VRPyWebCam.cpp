#include "VRPyWebCam.h"
#include "core/scripting/VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRWebCam>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Webcam",             /*tp_name*/
    sizeof(VRPyWebCam),             /*tp_basicsize*/
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
    "Webcam binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyWebCam::methods,             /* tp_methods */
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

PyMethodDef VRPyWebCam::methods[] = {
    {"connect", (PyCFunction)VRPyWebCam::connect, METH_VARARGS, "Connect to a webcam raw stream - connect(str uri, int res, flt ratio)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyWebCam::connect(VRPyWebCam* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyWebCam::connect, obj is invalid"); return NULL; }
	PyObject* uri; int res; float ratio;
    if (! PyArg_ParseTuple(args, "Oif", &uri, &res, &ratio)) return NULL;
	self->obj->connect( PyString_AsString(uri), res, ratio);
    Py_RETURN_TRUE;
}
