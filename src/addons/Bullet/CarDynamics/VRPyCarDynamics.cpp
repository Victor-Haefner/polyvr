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
    {"setupSimpleWheels", (PyCFunction)VRPyCarDynamics::setupSimpleWheels, METH_VARARGS, "Setup classic wheels - setupSimpleWheels( geo, float X, float Zp, float Zn, float height, float radius, float width)" },
    {"setCarMass", (PyCFunction)VRPyCarDynamics::setCarMass, METH_VARARGS, "Set car weight, must be done before creating car." },
    {"reset", (PyCFunction)VRPyCarDynamics::reset, METH_VARARGS, "Reset car - reset([x,y,z])" },
    {"getSteering", (PyCFunction)VRPyCarDynamics::getSteering, METH_NOARGS, "Get car steering - float getSteering()" },
    {"getThrottle", (PyCFunction)VRPyCarDynamics::getThrottle, METH_NOARGS, "Get car throttle - float getThrottle()" },
    {"getBreaking", (PyCFunction)VRPyCarDynamics::getBreaking, METH_NOARGS, "Get car breaking - float getBreaking()" },
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
    for (uint i=0; i<wheels.size(); i++) PyList_SetItem(pyWheels, i, VRPyTransform::fromSharedPtr(wheels[i]));
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

PyObject* VRPyCarDynamics::setupSimpleWheels(VRPyCarDynamics* self, PyObject* args) {
    VRPyGeometry* geo = NULL;
    float X, Zp, Zn, h, r, w;
    if (! PyArg_ParseTuple(args, "Offffff", &geo, &X, &Zp, &Zn, &h, &r, &w)) return NULL;
    self->objPtr->setupSimpleWheels(geo->objPtr, X, Zp, Zn, h, r, w);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::setCarMass(VRPyCarDynamics* self, PyObject* args) {
	self->objPtr->setCarMass(parseFloat(args));
	Py_RETURN_TRUE;
}



