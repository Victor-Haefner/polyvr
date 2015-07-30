#include "VRPyHaptic.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

#include "VRPyTransform.h"

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
    0,             /* tp_members */
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

PyMethodDef VRPyHaptic::methods[] = {
    {"setSimulationScales", (PyCFunction)VRPyHaptic::setSimulationScales, METH_VARARGS, "Set force on haptic device" },
    {"setForce", (PyCFunction)VRPyHaptic::setForce, METH_VARARGS, "Set force on haptic device" },
    {"getForce", (PyCFunction)VRPyHaptic::getForce, METH_NOARGS, "get 3-Tuple positional force the user generated" },
    {"attachTransform", (PyCFunction)VRPyHaptic::attachTransform, METH_VARARGS, "attaches given Transform to the virtuose (Command-Mode has to be COMMAND_MODE_VIRTMECH) Gravity for this transform should be set to zero,"},
    {"detachTransform", (PyCFunction)VRPyHaptic::detachTransform, METH_NOARGS, "detach previously attached transform" },
    {"getButtonStates",(PyCFunction)VRPyHaptic::getButtonStates, METH_NOARGS,"return a 3-Tuple with the states of virtuose's three buttons. e.g. (0,0,1) means the 3rd button is active, the others not"},
    {"setBase",(PyCFunction)VRPyHaptic::setBase, METH_VARARGS,"sets the given transform to the representation of the haptic's base"},
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



PyObject* VRPyHaptic::getButtonStates(VRPyHaptic* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyHaptic::getButtonStates - Object is invalid"); return NULL; }
    OSG::Vec3i states = self->obj->getButtonStates();
    return toPyTuple(states);
}

PyObject* VRPyHaptic::getForce(VRPyHaptic* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyHaptic::getForce - Object is invalid"); return NULL; }
    OSG::Vec3f force = self->obj->getForce();
    return toPyTuple(force);
}

PyObject* VRPyHaptic::attachTransform(VRPyHaptic* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyHaptic::attachTransform - Object is invalid"); return NULL; }
    VRPyTransform* tr;
    if (! PyArg_ParseTuple(args, "O", &tr)) return NULL;
    self->obj->attachTransform(tr->obj);
    Py_RETURN_TRUE;
}
PyObject* VRPyHaptic::setBase(VRPyHaptic* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyHaptic::setBase - Object is invalid"); return NULL; }
    VRPyTransform* tr;
    if (! PyArg_ParseTuple(args, "O", &tr)) return NULL;
    self->obj->setBase(tr->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyHaptic::detachTransform(VRPyHaptic* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyHaptic::updateHapticToObject - Object is invalid"); return NULL; }
    self->obj->detachTransform();
    Py_RETURN_TRUE;
}




