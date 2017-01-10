#include "VRPyCarDynamics.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyPose.h"
#include "core/scripting/VRPyPath.h"

using namespace OSG;

simplePyType(CarDynamics, New_named_ptr);

PyMethodDef VRPyCarDynamics::methods[] = {
    {"update", (PyCFunction)VRPyCarDynamics::update, METH_VARARGS, "Update vehicle physics input (float throttle {0,1}, float break {0,1}, float steering {-1,1})" },
    {"setChassis", (PyCFunction)VRPyCarDynamics::setChassis, METH_VARARGS, "Set chassis geometry" },
    {"setWheel", (PyCFunction)VRPyCarDynamics::setWheel, METH_VARARGS, "Set wheel geometry" },
    {"setCarMass", (PyCFunction)VRPyCarDynamics::setCarMass, METH_VARARGS, "Set car weight, must be done before creating car." },
    {"setWheelParams", (PyCFunction)VRPyCarDynamics::setWheelParams, METH_VARARGS, "Set wheel parameters - setWheelParams(width, radius)" },
    {"setWheelOffsets", (PyCFunction)VRPyCarDynamics::setWheelOffsets, METH_VARARGS, "Set wheel offsets - setWheelOffsets(x offset, front offset, rear offset, height)" },
    {"reset", (PyCFunction)VRPyCarDynamics::reset, METH_VARARGS, "Reset car - reset([x,y,z])" },
    {"getSteering", (PyCFunction)VRPyCarDynamics::getSteering, METH_NOARGS, "Get car steering - getSteering()" },
    {"getThrottle", (PyCFunction)VRPyCarDynamics::getThrottle, METH_NOARGS, "Get car throttle - getThrottle()" },
    {"getBreaking", (PyCFunction)VRPyCarDynamics::getBreaking, METH_NOARGS, "Get car breaking - getBreaking()" },
    {"getSpeed", (PyCFunction)VRPyCarDynamics::getSpeed, METH_NOARGS, "Get car speed" },
    {"getAcceleration", (PyCFunction)VRPyCarDynamics::getAcceleration, METH_NOARGS, "Get car acceleration" },
    {"getRoot", (PyCFunction)VRPyCarDynamics::getRoot, METH_NOARGS, "Get car root node" },
    {"getChassis", (PyCFunction)VRPyCarDynamics::getChassis, METH_NOARGS, "Get car chassis" },
    {"getWheels", (PyCFunction)VRPyCarDynamics::getWheels, METH_NOARGS, "Get car wheels" },
    {"followPath", (PyCFunction)VRPyCarDynamics::followPath, METH_VARARGS, "Start the pilot to follow a path with a certain speed curve - followPath( path p, path v )" },
    {"stopPilot", (PyCFunction)VRPyCarDynamics::stopPilot, METH_NOARGS, "Stop auto pilot - stopPilot()" },
    {"onAutoPilot", (PyCFunction)VRPyCarDynamics::onAutoPilot, METH_NOARGS, "Check if auto pilot - bool onAutoPilot()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyCarDynamics::getWheels(VRPyCarDynamics* self) {
    auto wheels = self->objPtr->getWheels();
    PyObject* pyWheels = PyList_New(wheels.size());
    for (int i=0; i<wheels.size(); i++) PyList_SetItem(pyWheels, i, VRPyTransform::fromSharedPtr(wheels[i]));
    return pyWheels;
}

PyObject* VRPyCarDynamics::onAutoPilot(VRPyCarDynamics* self) {
    return PyBool_FromLong( self->objPtr->onAutoPilot() );
}

PyObject* VRPyCarDynamics::getSteering(VRPyCarDynamics* self) {
    return PyFloat_FromDouble(self->objPtr->getSteering());
}

PyObject* VRPyCarDynamics::getThrottle(VRPyCarDynamics* self) {
    return PyFloat_FromDouble(self->objPtr->getThrottle());
}

PyObject* VRPyCarDynamics::getBreaking(VRPyCarDynamics* self) {
    return PyFloat_FromDouble(self->objPtr->getBreaking());
}

PyObject* VRPyCarDynamics::getChassis(VRPyCarDynamics* self) {
    return VRPyTransform::fromSharedPtr(self->objPtr->getChassis());
}

PyObject* VRPyCarDynamics::followPath(VRPyCarDynamics* self, PyObject* args) {
    VRPyPath *p,*v;
    if (! PyArg_ParseTuple(args, "OO", &p, &v)) return NULL;
    self->objPtr->followPath(p->objPtr, v->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::stopPilot(VRPyCarDynamics* self) {
    self->objPtr->stopPilot();
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::getRoot(VRPyCarDynamics* self) {
    return VRPyObject::fromSharedPtr(self->objPtr->getRoot());
}

PyObject* VRPyCarDynamics::getSpeed(VRPyCarDynamics* self) {
    return PyFloat_FromDouble(self->objPtr->getSpeed());
}

PyObject* VRPyCarDynamics::getAcceleration(VRPyCarDynamics* self) {
    return PyFloat_FromDouble(self->objPtr->getAcceleration());
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
