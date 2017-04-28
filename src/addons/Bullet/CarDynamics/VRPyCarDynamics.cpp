#include "VRPyCarDynamics.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyPose.h"
#include "core/scripting/VRPyPath.h"
#include "core/scripting/VRPySound.h"
#include "addons/Bullet/CarDynamics/CarSound/CarSound.h"

using namespace OSG;

simplePyType(CarDynamics, New_named_ptr);
simpleVRPyType(Driver, New_ptr);

PyMethodDef VRPyCarDynamics::methods[] = {
    {"update", (PyCFunction)VRPyCarDynamics::update, METH_VARARGS, "Update vehicle physics input (float throttle {-1,1}, float break {0,1}, float steering {-1,1}, float clutch {0,1}, int gear)" },
    {"setChassis", (PyCFunction)VRPyCarDynamics::setChassis, METH_VARARGS, "Set chassis geometry" },
    {"setupSimpleWheels", (PyCFunction)VRPyCarDynamics::setupSimpleWheels, METH_VARARGS, "Setup classic wheels - setupSimpleWheels( geo, float X, float Zp, float Zn, float height, float radius, float width)" },
    {"setParameter", (PyCFunction)VRPyCarDynamics::setParameter, METH_VARARGS, "Set car parameter, must be done before creating car - setParameter( float mass, float max_steering, float engine_power, float break_power )" },
    {"reset", (PyCFunction)VRPyCarDynamics::reset, METH_VARARGS, "Reset car - reset([x,y,z])" },
    {"getSteering", (PyCFunction)VRPyCarDynamics::getSteering, METH_NOARGS, "Get car steering - float getSteering()" },
    {"getThrottle", (PyCFunction)VRPyCarDynamics::getThrottle, METH_NOARGS, "Get car throttle - float getThrottle()" },
    {"getBreaking", (PyCFunction)VRPyCarDynamics::getBreaking, METH_NOARGS, "Get car breaking - float getBreaking()" },
    {"getClutch", (PyCFunction)VRPyCarDynamics::getClutch, METH_NOARGS, "Get car clutch - float getClutch()" },
    {"getSpeed", (PyCFunction)VRPyCarDynamics::getSpeed, METH_NOARGS, "Get car speed" },
    {"getAcceleration", (PyCFunction)VRPyCarDynamics::getAcceleration, METH_NOARGS, "Get car acceleration" },
    {"getRoot", (PyCFunction)VRPyCarDynamics::getRoot, METH_NOARGS, "Get car root node" },
    {"getChassis", (PyCFunction)VRPyCarDynamics::getChassis, METH_NOARGS, "Get car chassis" },
    {"getWheels", (PyCFunction)VRPyCarDynamics::getWheels, METH_NOARGS, "Get car wheels" },
    {"getRPM", (PyCFunction)VRPyCarDynamics::getRPM, METH_NOARGS, "Get car RPM" },
    {"isRunning", (PyCFunction)VRPyCarDynamics::isRunning, METH_NOARGS, "Is car engine running - bool isRunning()" },
    {"getGear", (PyCFunction)VRPyCarDynamics::getGear, METH_NOARGS, "Get car gear" },
    {"setIgnition", (PyCFunction)VRPyCarDynamics::setIgnition, METH_VARARGS, "Set ignition - setIgnition(bool)" },
    {"loadCarSound", (PyCFunction)VRPyCarDynamics::loadCarSound, METH_VARARGS, "Load car sound dataset - loadCarSound(filename)" },
    {"toggleCarSound", (PyCFunction)VRPyCarDynamics::toggleCarSound, METH_VARARGS, "toggle car sound - toggleCarSound(bool)" },
    {"getCarSound", (PyCFunction)VRPyCarDynamics::getCarSound, METH_NOARGS, "Get car sound - getCarSound()" },
    {"carSoundIsLoaded", (PyCFunction)VRPyCarDynamics::carSoundIsLoaded, METH_NOARGS, "Query if audio data has been loaded - carSoundIsLoaded()" },
    {"setFade", (PyCFunction)VRPyCarDynamics::setFade, METH_VARARGS, "Set ignition - setFade(float)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyCarDynamics::loadCarSound(VRPyCarDynamics* self, PyObject* args) {
    const char* t = 0;
    if (! PyArg_ParseTuple(args, "s", &t)) return NULL;
    self->objPtr->getCarSound()->loadSoundFile(t?t:"");
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::toggleCarSound(VRPyCarDynamics* self, PyObject* args) {
    int b;
    if (! PyArg_ParseTuple(args, "i", &b)) return NULL;
    self->objPtr->getCarSound()->toggleSound(b);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::getCarSound(VRPyCarDynamics* self) {
    return VRPySound::fromSharedPtr( self->objPtr->getCarSound()->getSound() );
}

PyObject* VRPyCarDynamics::carSoundIsLoaded(VRPyCarDynamics* self) {
    return PyInt_FromLong(self->objPtr->getCarSound()->isLoaded());
}

PyObject* VRPyCarDynamics::setFade(VRPyCarDynamics* self, PyObject* args) {
    float f, d;
    if (! PyArg_ParseTuple(args, "ff", &d, &f)) return NULL;
    self->objPtr->getCarSound()->setFade(f);
    self->objPtr->getCarSound()->setDuration(d);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::getWheels(VRPyCarDynamics* self) {
    auto wheels = self->objPtr->getWheels();
    PyObject* pyWheels = PyList_New(wheels.size());
    for (uint i=0; i<wheels.size(); i++) PyList_SetItem(pyWheels, i, VRPyTransform::fromSharedPtr(wheels[i]));
    return pyWheels;
}


PyObject* VRPyCarDynamics::getRPM(VRPyCarDynamics* self) {
    return PyInt_FromLong(self->objPtr->getRPM());
}

PyObject* VRPyCarDynamics::isRunning(VRPyCarDynamics* self) {
    return PyInt_FromLong(self->objPtr->isRunning());
}

PyObject* VRPyCarDynamics::getGear(VRPyCarDynamics* self) {
    return PyInt_FromLong(self->objPtr->getGear());
}

PyObject* VRPyCarDynamics::getClutch(VRPyCarDynamics* self) {
    return PyFloat_FromDouble(self->objPtr->getClutch());
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

PyObject* VRPyCarDynamics::setIgnition(VRPyCarDynamics* self, PyObject* args) {
    int b;
    if (! PyArg_ParseTuple(args, "i", &b)) return NULL;
    self->objPtr->setIgnition(b);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::update(VRPyCarDynamics* self, PyObject* args) {
    float t,b,s,c;
    int g;
    if (! PyArg_ParseTuple(args, "ffffi", &t, &b, &s, &c, &g)) return NULL;
    self->objPtr->setThrottle(t);
    self->objPtr->setBreak(b);
    self->objPtr->setSteering(s);
    self->objPtr->setClutch(c);
    self->objPtr->setGear(g);
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

PyObject* VRPyCarDynamics::setParameter(VRPyCarDynamics* self, PyObject* args) {
    float m, s, e, b;
    if (! PyArg_ParseTuple(args, "ffff", &m, &s, &e, &b)) return NULL;
	self->objPtr->setParameter(m,s,e,b);
	Py_RETURN_TRUE;
}


PyMethodDef VRPyDriver::methods[] = {
    {"setCar", (PyCFunction)VRPyDriver::setCar, METH_VARARGS, "Set car - setCar( car )" },
    {"followPath", (PyCFunction)VRPyDriver::followPath, METH_VARARGS, "Start the pilot to follow a path with a certain speed curve - followPath( path p, path v )" },
    {"stop", (PyCFunction)VRPyDriver::stop, METH_NOARGS, "Stop driving - stop()" },
    {"isDriving", (PyCFunction)VRPyDriver::isDriving, METH_NOARGS, "Check if driving - bool isDriving()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyDriver::isDriving(VRPyDriver* self) {
    return PyBool_FromLong( self->objPtr->isDriving() );
}

PyObject* VRPyDriver::setCar(VRPyDriver* self, PyObject* args) {
    VRPyCarDynamics* c;
    if (! PyArg_ParseTuple(args, "O", &c)) return NULL;
    self->objPtr->setCar(c->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyDriver::followPath(VRPyDriver* self, PyObject* args) {
    VRPyPath *p,*v;
    if (! PyArg_ParseTuple(args, "OO", &p, &v)) return NULL;
    self->objPtr->followPath(p->objPtr, v->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyDriver::stop(VRPyDriver* self) {
    self->objPtr->stop();
    Py_RETURN_TRUE;
}





