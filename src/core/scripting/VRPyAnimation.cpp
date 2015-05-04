#include "VRPyAnimation.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

#include "core/scene/VRAnimationManagerT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRAnimation<float> >::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Animation",             /*tp_name*/
    sizeof(VRPyAnimation),             /*tp_basicsize*/
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
    "Animation binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyAnimation::methods,             /* tp_methods */
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

PyMethodDef VRPyAnimation::methods[] = {
    {"start", (PyCFunction)VRPyAnimation::start, METH_NOARGS, "Start animation" },
    {"stop", (PyCFunction)VRPyAnimation::stop, METH_NOARGS, "Stop animation" },
    {"isActive", (PyCFunction)VRPyAnimation::isActive, METH_NOARGS, "Check if running - bool isActive()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyAnimation::start(VRPyAnimation* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyAnimation::start - Object is invalid"); return NULL; }
    self->obj->start();
    Py_RETURN_TRUE;
}

PyObject* VRPyAnimation::stop(VRPyAnimation* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyAnimation::stop - Object is invalid"); return NULL; }
    self->obj->stop();
    Py_RETURN_TRUE;
}

PyObject* VRPyAnimation::isActive(VRPyAnimation* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyAnimation::isActive - Object is invalid"); return NULL; }
    return PyBool_FromLong( self->obj->isActive() );
}


