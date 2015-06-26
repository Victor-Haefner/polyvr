#include "VRPyCarDynamics.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::CarDynamics>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.CarDynamics",             /*tp_name*/
    sizeof(VRPyCarDynamics),             /*tp_basicsize*/
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
    "VRCarDynamics binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyCarDynamics::methods,             /* tp_methods */
    VRPyCarDynamics::members,             /* tp_members */
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

PyMemberDef VRPyCarDynamics::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyCarDynamics::methods[] = {
    {"update", (PyCFunction)VRPyCarDynamics::update, METH_VARARGS, "Update vehicle physics input (throttle force, break, steering [-1,1])" },
    {"setChassis", (PyCFunction)VRPyCarDynamics::setChassis, METH_VARARGS, "Set chassis geometry" },
    {"setWheel", (PyCFunction)VRPyCarDynamics::setWheel, METH_VARARGS, "Set wheel geometry" },
    {"setCarMass", (PyCFunction)VRPyCarDynamics::setCarMass, METH_VARARGS, "Set car weight, must be done before creating car." },
    {"setWheelParams", (PyCFunction)VRPyCarDynamics::setWheelParams, METH_VARARGS, "Set wheel parameters, -1 uses default valuen\tpos 0 = widthn\tpos 1 = radius" },
    {"setWheelOffsets", (PyCFunction)VRPyCarDynamics::setWheelOffsets, METH_VARARGS, "Set wheel offsets, -1 sets default value\n\tpos 0 = xOffset\n\tpos 1 = frontZOffset\n\tpos 2 = rearZOffset\n\tpos 3 = height" },
    {"reset", (PyCFunction)VRPyCarDynamics::reset, METH_VARARGS, "Reset car - reset([x,y,z])" },
    {"getSpeed", (PyCFunction)VRPyCarDynamics::getSpeed, METH_NOARGS, "Get car speed" },
    {"getRoot", (PyCFunction)VRPyCarDynamics::getRoot, METH_NOARGS, "Get car root node" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyCarDynamics::getRoot(VRPyCarDynamics* self) {
    if (self->obj == 0) self->obj = new OSG::CarDynamics();
    return VRPyObject::fromPtr(self->obj->getRoot());
}

PyObject* VRPyCarDynamics::getSpeed(VRPyCarDynamics* self) {
    if (self->obj == 0) self->obj = new OSG::CarDynamics();
    return PyFloat_FromDouble(self->obj->getSpeed());
}

PyObject* VRPyCarDynamics::reset(VRPyCarDynamics* self, PyObject* args) {
    if (self->obj == 0) self->obj = new OSG::CarDynamics();
    auto v = parseVec3f(args);
    self->obj->reset(v[0], v[1], v[2]);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::update(VRPyCarDynamics* self, PyObject* args) {
    OSG::Vec3f input = parseVec3f(args);

    if (self->obj == 0) self->obj = new OSG::CarDynamics();
    self->obj->setThrottle(input[0]);
    self->obj->setBreak(input[1]);
    self->obj->setSteering(input[2]);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::setChassis(VRPyCarDynamics* self, PyObject* args) {
    PyObject* dev = NULL;
    if (! PyArg_ParseTuple(args, "O", &dev)) return NULL;
    if (dev == NULL) { PyErr_SetString(err, "Missing device parameter"); return NULL; }
    VRPyGeometry* _dev = (VRPyGeometry*)dev;

    if (self->obj == 0) self->obj = new OSG::CarDynamics();
    self->obj->setChassisGeo(_dev->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::setWheel(VRPyCarDynamics* self, PyObject* args) {
    PyObject* dev = NULL;
    if (! PyArg_ParseTuple(args, "O", &dev)) return NULL;
    if (dev == NULL) { PyErr_SetString(err, "Missing device parameter"); return NULL; }
    VRPyGeometry* _dev = (VRPyGeometry*)dev;

    if (self->obj == 0) self->obj = new OSG::CarDynamics();
    self->obj->setWheelGeo(_dev->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::setCarMass(VRPyCarDynamics* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setCarMass, C obj is invalid"); return NULL; }
    cout << "setting car mass" << endl;
	self->obj->setCarMass(parseFloat(args));
	cout << "setting car mass" << endl;
	Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::setWheelOffsets(VRPyCarDynamics* self, PyObject* args){
    float b1, b2, b3,b4;
    if (! PyArg_ParseTuple(args, "ffff", &b1, &b2, &b3,&b4)) return NULL;

    if (self->obj == 0) self->obj = new OSG::CarDynamics();
    self->obj->setWheelOffsets(b1, b2, b3,b4);

    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::setWheelParams(VRPyCarDynamics* self, PyObject* args){
    float b1, b2;
    if (! PyArg_ParseTuple(args, "ff", &b1, &b2)) return NULL;

    if (self->obj == 0) self->obj = new OSG::CarDynamics();
    self->obj->setWheelParams(b1, b2);

    Py_RETURN_TRUE;
}
