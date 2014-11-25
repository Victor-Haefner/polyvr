#include "VRPyHaptic.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRHaptic>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Haptic",             /*tp_name*/
    sizeof(VRPyHaptic),             /*tp_basicsize*/
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
    "Haptic binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyHaptic::methods,             /* tp_methods */
    VRPyHaptic::members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
};

PyMemberDef VRPyHaptic::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyHaptic::methods[] = {
    {"setSimulationScales", (PyCFunction)VRPyHaptic::setSimulationScales, METH_VARARGS, "Set force on haptic device" },
    {"setForce", (PyCFunction)VRPyHaptic::setForce, METH_VARARGS, "Set force on haptic device" },
    {"connectPhysicalized", (PyCFunction)VRPyHaptic::connectPhysicalized, METH_VARARGS, "connect a physicalized object to the haptic's avatar" },
    {"updateConnectedObjects", (PyCFunction)VRPyHaptic::updateConnectedObjects, METH_VARARGS, "update the translation of the connected objects" },
    {"updateFeedbackForces", (PyCFunction)VRPyHaptic::updateFeedbackForces, METH_VARARGS, "update the forces provided by the connected objects on the haptic" },
    {NULL}  /* Sentinel */
};


PyObject* VRPyHaptic::setSimulationScales(VRPyHaptic* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyHaptic::setSimulationScales - Object is invalid"); return NULL; }
    OSG::Vec2f v = parseVec2f(args);
    self->obj->setSimulationScales(v[0], v[1]);
    Py_RETURN_TRUE;
}

PyObject* VRPyHaptic::setForce(VRPyHaptic* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyHaptic::setForce - Object is invalid"); return NULL; }
    float x,y,z,u,v,w;
    if (! PyArg_ParseTuple(args, "ffffff", &x, &y, &z, &u, &v, &w)) return NULL;
    self->obj->setForce(OSG::Vec3f(x,y,z), OSG::Vec3f(u,v,w));
    Py_RETURN_TRUE;
}


PyObject* VRPyHaptic::connectPhysicalized(VRPyHaptic* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyHaptic::connectPhysicalized - Object is invalid"); return NULL; }
    VRPhysics* tr;
    self->obj->connectPhysicalized(tr);
    if (! PyArg_ParseTuple(args, "ffffff", &tr)) return NULL;
    Py_RETURN_TRUE;
}


PyObject* VRPyHaptic::updateConnectedObjects(VRPyHaptic* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyHaptic::updateConnected - Object is invalid"); return NULL; }
    VRPhysics* tr;
    if (! PyArg_ParseTuple(args, "ffffff", &tr)) return NULL;
    self->obj->updateConnectedObjects();
    Py_RETURN_TRUE;
}

PyObject* VRPyHaptic::updateFeedbackForces(VRPyHaptic* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyHaptic::updateFeedbackForces - Object is invalid"); return NULL; }
    VRPhysics* tr;
    if (! PyArg_ParseTuple(args, "ffffff", &tr)) return NULL;
    self->obj->updateFeedbackForces();
    Py_RETURN_TRUE;
}


