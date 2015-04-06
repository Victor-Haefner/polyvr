#include "VRPyMillingMachine.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyTransform.h"


template<> PyTypeObject VRPyBaseT<OSG::VRMillingMachine>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.MillingMachine",             /*tp_name*/
    sizeof(VRPyMillingMachine),             /*tp_basicsize*/
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
    "MillingMachine binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyMillingMachine::methods,             /* tp_methods */
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

PyMethodDef VRPyMillingMachine::methods[] = {
    {"connect", (PyCFunction)VRPyMillingMachine::connect, METH_VARARGS, "Connect to machine - connect( 'ip:port' )" },
    {"disconnect", (PyCFunction)VRPyMillingMachine::disconnect, METH_NOARGS, "Disconnect from machine" },
    {"connected", (PyCFunction)VRPyMillingMachine::connected, METH_NOARGS, "Return if machine connected - bool connected()" },
    {"setSpeed", (PyCFunction)VRPyMillingMachine::setSpeed, METH_VARARGS, "Set milling speed and direction - setSpeed( [x,y,z] )" },
    {"setGeometry", (PyCFunction)VRPyMillingMachine::setGeometry, METH_VARARGS, "Set the geometries - setGeometry( [objX, objY, objZ, endEffector] )" },
    {"update", (PyCFunction)VRPyMillingMachine::update, METH_NOARGS, "Update the machine visualisation - update()" },
    {"setPosition", (PyCFunction)VRPyMillingMachine::setPosition, METH_VARARGS, "Set the position of the machine - setPosition( [x,y,z] )" },
    {"getPosition", (PyCFunction)VRPyMillingMachine::setPosition, METH_NOARGS, "Get the position of the machine - [x,y,z] getPosition()" },
    {"state", (PyCFunction)VRPyMillingMachine::state, METH_NOARGS, "Get the state of the machine" },
    {"mode", (PyCFunction)VRPyMillingMachine::mode, METH_NOARGS, "Get the mode of the machine" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMillingMachine::connected(VRPyMillingMachine* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMillingMachine::connected - Object is invalid"); return NULL; }
    return PyBool_FromLong( self->obj->connected() );
}

PyObject* VRPyMillingMachine::getPosition(VRPyMillingMachine* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMillingMachine::getPosition - Object is invalid"); return NULL; }
    return toPyTuple( self->obj->getPosition() );
}

PyObject* VRPyMillingMachine::state(VRPyMillingMachine* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMillingMachine::state - Object is invalid"); return NULL; }
    return PyInt_FromLong( self->obj->getState() );
}

PyObject* VRPyMillingMachine::mode(VRPyMillingMachine* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMillingMachine::mode - Object is invalid"); return NULL; }
    return PyInt_FromLong( self->obj->getMode() );
}

PyObject* VRPyMillingMachine::update(VRPyMillingMachine* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMillingMachine::update - Object is invalid"); return NULL; }
    self->obj->update();
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingMachine::setPosition(VRPyMillingMachine* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMillingMachine::setPosition - Object is invalid"); return NULL; }
    self->obj->setPosition( parseVec3f(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingMachine::setGeometry(VRPyMillingMachine* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMillingMachine::setGeometry - Object is invalid"); return NULL; }
    vector<OSG::VRTransform*> geos;
    vector<PyObject*> objs = parseList(args);
    for (auto o : objs) {
        OSG::VRTransform* g = ((VRPyTransform*)o)->obj;
        geos.push_back(g);
    }
    self->obj->setGeometry(geos);
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingMachine::connect(VRPyMillingMachine* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMillingMachine::connect - Object is invalid"); return NULL; }
    self->obj->connect( parseString(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingMachine::disconnect(VRPyMillingMachine* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMillingMachine::disconnect - Object is invalid"); return NULL; }
    self->obj->disconnect();
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingMachine::setSpeed(VRPyMillingMachine* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMillingMachine::setSpeed - Object is invalid"); return NULL; }
    self->obj->setSpeed( parseVec3f(args) );
    Py_RETURN_TRUE;
}
