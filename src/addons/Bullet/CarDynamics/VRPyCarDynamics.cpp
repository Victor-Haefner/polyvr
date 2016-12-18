#include "VRPyCarDynamics.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyPose.h"

using namespace OSG;

simplePyType(CarDynamics, New_named_ptr);

PyMethodDef VRPyCarDynamics::methods[] = {
    {"update", (PyCFunction)VRPyCarDynamics::update, METH_VARARGS, "Update vehicle physics input (throttle force, break, steering [-1,1])" },
    {"setChassis", (PyCFunction)VRPyCarDynamics::setChassis, METH_VARARGS, "Set chassis geometry" },
    {"setWheel", (PyCFunction)VRPyCarDynamics::setWheel, METH_VARARGS, "Set wheel geometry" },
    {"setCarMass", (PyCFunction)VRPyCarDynamics::setCarMass, METH_VARARGS, "Set car weight, must be done before creating car." },
    {"setWheelParams", (PyCFunction)VRPyCarDynamics::setWheelParams, METH_VARARGS, "Set wheel parameters - setWheelParams(width, radius)" },
    {"setWheelOffsets", (PyCFunction)VRPyCarDynamics::setWheelOffsets, METH_VARARGS, "Set wheel offsets - setWheelOffsets(x offset, front offset, rear offset, height)" },
    {"reset", (PyCFunction)VRPyCarDynamics::reset, METH_VARARGS, "Reset car - reset([x,y,z])" },
    {"getSpeed", (PyCFunction)VRPyCarDynamics::getSpeed, METH_NOARGS, "Get car speed" },
    {"getRoot", (PyCFunction)VRPyCarDynamics::getRoot, METH_NOARGS, "Get car root node" },
    {"getChassis", (PyCFunction)VRPyCarDynamics::getChassis, METH_NOARGS, "Get car chassis" },
    {"getWheels", (PyCFunction)VRPyCarDynamics::getWheels, METH_NOARGS, "Get car wheels" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyCarDynamics::getWheels(VRPyCarDynamics* self) {
    auto wheels = self->objPtr->getWheels();
    PyObject* pyWheels = PyList_New(wheels.size());
    for (int i=0; i<wheels.size(); i++) PyList_SetItem(pyWheels, i, VRPyTransform::fromSharedPtr(wheels[i]));
    return pyWheels;
}

PyObject* VRPyCarDynamics::getChassis(VRPyCarDynamics* self) {
    return VRPyTransform::fromSharedPtr(self->objPtr->getChassis());
}

PyObject* VRPyCarDynamics::getRoot(VRPyCarDynamics* self) {
    return VRPyObject::fromSharedPtr(self->objPtr->getRoot());
}

PyObject* VRPyCarDynamics::getSpeed(VRPyCarDynamics* self) {
    return PyFloat_FromDouble(self->objPtr->getSpeed());
}

PyObject* VRPyCarDynamics::reset(VRPyCarDynamics* self, PyObject* args) {
    VRPyPose* p;
    if (! PyArg_ParseTuple(args, "O", &p)) return NULL;
    self->objPtr->reset(*p->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::update(VRPyCarDynamics* self, PyObject* args) {
    OSG::Vec3f input = parseVec3f(args);
    self->objPtr->setThrottle(input[0]);
    self->objPtr->setBreak(input[1]);
    self->objPtr->setSteering(input[2]);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::setChassis(VRPyCarDynamics* self, PyObject* args) {
    VRPyGeometry* dev = NULL; int doPhys = 1;
    if (! PyArg_ParseTuple(args, "O|i", &dev, &doPhys)) return NULL;
    self->objPtr->setChassisGeo(dev->objPtr, doPhys);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::setWheel(VRPyCarDynamics* self, PyObject* args) {
    VRPyGeometry* dev = NULL;
    if (! PyArg_ParseTuple(args, "O", &dev)) return NULL;
    self->objPtr->setWheelGeo(dev->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::setCarMass(VRPyCarDynamics* self, PyObject* args) {
    cout << "setting car mass" << endl;
	self->objPtr->setCarMass(parseFloat(args));
	cout << "setting car mass" << endl;
	Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::setWheelOffsets(VRPyCarDynamics* self, PyObject* args){
    float b1, b2, b3,b4;
    if (! PyArg_ParseTuple(args, "ffff", &b1, &b2, &b3,&b4)) return NULL;
    self->objPtr->setWheelOffsets(b1, b2, b3,b4);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::setWheelParams(VRPyCarDynamics* self, PyObject* args){
    float b1, b2;
    if (! PyArg_ParseTuple(args, "ff", &b1, &b2)) return NULL;
    self->objPtr->setWheelParams(b1, b2);
    Py_RETURN_TRUE;
}
