#include "VRPyDevice.h"
#include "VRPyTransform.h"
#include "VRPyGeometry.h"
#include "VRPySprite.h"
#include "core/objects/VRTransform.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

template<> PyTypeObject VRPyBaseT<OSG::VRDevice>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Device",             /*tp_name*/
    sizeof(VRPyDevice),             /*tp_basicsize*/
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
    "VRDevice binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyDevice::methods,             /* tp_methods */
    VRPyDevice::members,             /* tp_members */
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

PyMemberDef VRPyDevice::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyDevice::methods[] = {
    {"getName", (PyCFunction)VRPyDevice::getName, METH_NOARGS, "Return device name." },
    {"destroy", (PyCFunction)VRPyDevice::destroy, METH_NOARGS, "Destroy device." },
    {"getBeacon", (PyCFunction)VRPyDevice::getBeacon, METH_NOARGS, "Get device beacon." },
    {"setBeacon", (PyCFunction)VRPyDevice::setBeacon, METH_VARARGS, "Set device beacon." },
    {"getTarget", (PyCFunction)VRPyDevice::getTarget, METH_NOARGS, "Get device target." },
    {"setTarget", (PyCFunction)VRPyDevice::setTarget, METH_VARARGS, "Set device target." },
    {"getKey", (PyCFunction)VRPyDevice::getKey, METH_NOARGS, "Get activated device key." },
    {"getState", (PyCFunction)VRPyDevice::getState, METH_NOARGS, "Get device state." },
    {"getKeyState", (PyCFunction)VRPyDevice::getKeyState, METH_VARARGS, "Get device key state." },
    {"getSlider", (PyCFunction)VRPyDevice::getSlider, METH_VARARGS, "Get device slider state." },
    {"getMessage", (PyCFunction)VRPyDevice::getMessage, METH_NOARGS, "Get device received message." },
    {"getType", (PyCFunction)VRPyDevice::getType, METH_NOARGS, "Get device type." },
    {"setDnD", (PyCFunction)VRPyDevice::setDnD, METH_VARARGS, "Set drag && drop." },
    {"intersect", (PyCFunction)VRPyDevice::intersect, METH_NOARGS, "Intersects the scene" },
    {"getIntersected", (PyCFunction)VRPyDevice::getIntersected, METH_NOARGS, "Get device intersected object." },
    {"getIntersection", (PyCFunction)VRPyDevice::getIntersection, METH_NOARGS, "Get device intersection point." },
    {"getIntersectionNormal", (PyCFunction)VRPyDevice::getIntersectionNormal, METH_NOARGS, "Get normal at intersection point." },
    {"getIntersectionUV", (PyCFunction)VRPyDevice::getIntersectionUV, METH_NOARGS, "Get uv at intersection point." },
    {"addIntersection", (PyCFunction)VRPyDevice::addIntersection, METH_VARARGS, "Add device intersection node." },
    {"remIntersection", (PyCFunction)VRPyDevice::remIntersection, METH_VARARGS, "Remove device intersection node." },
    {"getDragged", (PyCFunction)VRPyDevice::getDragged, METH_NOARGS, "Get dragged object." },
    {"getDragGhost", (PyCFunction)VRPyDevice::getDragGhost, METH_NOARGS, "Get drag ghost." },
    {"drag", (PyCFunction)VRPyDevice::drag, METH_VARARGS, "Start to drag an object - drag(obj)" },
    {"drop", (PyCFunction)VRPyDevice::drop, METH_NOARGS, "Drop any object - drop()" },
    {"setSpeed", (PyCFunction)VRPyDevice::setSpeed, METH_VARARGS, "Drop any object - drop()" },
    {"getSpeed", (PyCFunction)VRPyDevice::getSpeed, METH_NOARGS, "Drop any object - drop()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyDevice::setSpeed(VRPyDevice* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::setSpeed, Object is invalid"); return NULL; }
    self->obj->setSpeed( parseVec2f(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::getSpeed(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getSpeed, Object is invalid"); return NULL; }
    return toPyTuple( self->obj->getSpeed() );
}

PyObject* VRPyDevice::intersect(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::intersect, Object is invalid"); return NULL; }
    self->obj->intersect(0);
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::drag(VRPyDevice* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::drag, Object is invalid"); return NULL; }
    OSG::VRObject* obj = 0;
    if (!VRPyObject::parse(args, &obj)) return NULL;
    string name = obj->getName();
    self->obj->drag(obj, self->obj->getBeacon());
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::drop(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::drop, Object is invalid"); return NULL; }
    self->obj->drop();
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::getName(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getName, Object is invalid"); return NULL; }
    return PyString_FromString(self->obj->getName().c_str());
}

PyObject* VRPyDevice::destroy(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::destroy, Object is invalid"); return NULL; }
    delete self->obj;
    self->obj = 0;
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::getBeacon(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getBeacon, Object is invalid"); return NULL; }
    return VRPyTransform::fromPtr(self->obj->getBeacon());
}

PyObject* VRPyDevice::setBeacon(VRPyDevice* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::setBeacon, Object is invalid"); return NULL; }
    PyObject* beacon = NULL;
    if (! PyArg_ParseTuple(args, "O", &beacon)) return NULL;
    VRPyTransform* t = (VRPyTransform*)beacon;
    self->obj->setBeacon(t->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::getTarget(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getTarget, Object is invalid"); return NULL; }
    return VRPyTransform::fromPtr(self->obj->getTarget());
}

PyObject* VRPyDevice::setTarget(VRPyDevice* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::setTarget, Object is invalid"); return NULL; }
    PyObject* target = NULL;
    if (! PyArg_ParseTuple(args, "O", &target)) return NULL;
    VRPyTransform* t = (VRPyTransform*)target;
    self->obj->setTarget(t->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::getState(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getState, Object is invalid"); return NULL; }
    return PyInt_FromLong(self->obj->getState());
}

PyObject* VRPyDevice::getKeyState(VRPyDevice* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getKeyState, Object is invalid"); return NULL; }
    int i=0;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    return PyInt_FromLong(self->obj->b_state(i));
}

PyObject* VRPyDevice::getKey(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getKey, Object is invalid"); return NULL; }
    return PyInt_FromLong(self->obj->key());
}

PyObject* VRPyDevice::getMessage(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getKey, Object is invalid"); return NULL; }
    return PyString_FromString(self->obj->getMessage().c_str());
}

PyObject* VRPyDevice::getSlider(VRPyDevice* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getSlider, Object is invalid"); return NULL; }
    int i=0;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    return PyFloat_FromDouble(self->obj->s_state(i));
}

PyObject* VRPyDevice::getType(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getType, Object is invalid"); return NULL; }
    return PyString_FromString(self->obj->getType().c_str());
}

PyObject* VRPyDevice::setDnD(VRPyDevice* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::setDnD, Object is invalid"); return NULL; }
    int i=0;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    self->obj->toggleDragnDrop(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::getIntersected(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getIntersected, Object is invalid"); return NULL; }
    return VRPyTypeCaster::cast(self->obj->getLastIntersection().object);
}

PyObject* VRPyDevice::getIntersection(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getIntersection, Object is invalid"); return NULL; }
    OSG::Pnt3f v = self->obj->getLastIntersection().point;
    return toPyTuple( OSG::Vec3f(v) );
}

PyObject* VRPyDevice::getIntersectionNormal(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getIntersectionNormal, Object is invalid"); return NULL; }
    OSG::Vec3f v = self->obj->getLastIntersection().normal;
    return toPyTuple(v);
}

PyObject* VRPyDevice::getIntersectionUV(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getIntersectionUV, Object is invalid"); return NULL; }
    OSG::Vec2f v = self->obj->getLastIntersection().texel;
    return toPyTuple(v);
}

PyObject* VRPyDevice::addIntersection(VRPyDevice* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::addIntersection, Object is invalid"); return NULL; }
    PyObject* iobj = NULL;
    if (! PyArg_ParseTuple(args, "O", &iobj)) return NULL;
    VRPyObject* t = (VRPyObject*)iobj;
    self->obj->addDynTree(t->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::remIntersection(VRPyDevice* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::remIntersection, Object is invalid"); return NULL; }
    PyObject* iobj = NULL;
    if (! PyArg_ParseTuple(args, "O", &iobj)) return NULL;
    VRPyObject* t = (VRPyObject*)iobj;
    self->obj->remDynTree(t->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::getDragGhost(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getDragGhost, Object is invalid"); return NULL; }
    return VRPyTypeCaster::cast(self->obj->getDraggedGhost());
}

PyObject* VRPyDevice::getDragged(VRPyDevice* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyDevice::getDragged, Object is invalid"); return NULL; }
    return VRPyTypeCaster::cast(self->obj->getDraggedObject());
}

