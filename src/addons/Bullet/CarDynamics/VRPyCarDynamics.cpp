#include "VRPyCarDynamics.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyPose.h"
#include "core/scripting/VRPyPath.h"
#include "core/scripting/VRPySound.h"
#ifndef WITHOUT_AV
#include "CarSound/CarSound.h"
#endif

using namespace OSG;

simpleVRPyType(CarDynamics, New_named_ptr);
#ifndef WITHOUT_AV
simpleVRPyType(CarSound, New_ptr);
#endif
simpleVRPyType(Driver, New_ptr);

#ifndef WITHOUT_AV
PyMethodDef VRPyCarSound::methods[] = {
    {"play", PyWrap(CarSound, play, "play, feed RPM", void, float) },
    {"loadSoundFile", PyWrap(CarSound, loadSoundFile, "load ressources", void, string) },
    {NULL}  /* Sentinel */
};
#endif

PyMethodDef VRPyCarDynamics::methods[] = {
    {"update", PyWrap(CarDynamics, update, "Update vehicle physics input, throttle {0,1}, break {0,1}, steering {-1,1}, clutch {0,1}, gear", void, float, float, float, float, int) },
    {"updateWheel", PyWrap(CarDynamics, updateWheel, "Update vehicle physics for a single wheel, wheel and parameter as in update", void, int, float, float, float, float, int) },
    {"setChassis", (PyCFunction)VRPyCarDynamics::setChassis, METH_VARARGS, "Set chassis geometry - setChassis( geo | bool physicalize) " },
    {"addWheel", PyWrap(CarDynamics, addWheel, "Add a wheel, geometry, position, radius, width, maxSteer, steered, driven", void, VRGeometryPtr, Vec3d, float, float, float, bool, bool) },
    {"setupSimpleWheels", (PyCFunction)VRPyCarDynamics::setupSimpleWheels, METH_VARARGS, "Setup classic wheels - setupSimpleWheels( geo, float X, float Zp, float Zn, float height, float radius, float width, float max_steering)" },
    {"setParameter", (PyCFunction)VRPyCarDynamics::setParameter, METH_VARARGS, "Set car parameter, must be done before creating car - setParameter( float mass, float engine_power, float break_power | [x,y,z] mass offset, bool enableStalling  )" },
    {"reset", (PyCFunction)VRPyCarDynamics::reset, METH_VARARGS, "Reset car - reset( pose )" },
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
    {"geteForce", (PyCFunction)VRPyCarDynamics::geteForce, METH_NOARGS, "geteForce" },
    {"geteBreak", (PyCFunction)VRPyCarDynamics::geteBreak, METH_NOARGS, "geteBreak" },
    {"isRunning", (PyCFunction)VRPyCarDynamics::isRunning, METH_NOARGS, "Is car engine running - bool isRunning()" },
    {"getGear", (PyCFunction)VRPyCarDynamics::getGear, METH_NOARGS, "Get car gear" },
    {"setIgnition", (PyCFunction)VRPyCarDynamics::setIgnition, METH_VARARGS, "Set ignition - setIgnition(bool)" },
#ifndef WITHOUT_AV
    {"loadCarSound", (PyCFunction)VRPyCarDynamics::loadCarSound, METH_VARARGS, "Load car sound dataset - loadCarSound(filename)" },
    {"toggleCarSound", (PyCFunction)VRPyCarDynamics::toggleCarSound, METH_VARARGS, "toggle car sound - toggleCarSound(bool)" },
    {"getCarSound", (PyCFunction)VRPyCarDynamics::getCarSound, METH_NOARGS, "Get car sound - getCarSound()" },
    {"carSoundIsLoaded", (PyCFunction)VRPyCarDynamics::carSoundIsLoaded, METH_NOARGS, "Query if audio data has been loaded - carSoundIsLoaded()" },
    {"setFade", (PyCFunction)VRPyCarDynamics::setFade, METH_VARARGS, "Set car sound fade - setFade( flt fade, flt duration )" },
#endif
    {"setType", (PyCFunction)VRPyCarDynamics::setType, METH_VARARGS, "Set car sound fade - setType( int type )\n\ttype can be 0 (simple), 1 (automatic), 2 (semiautomatic), 3 (manual)" },
    {NULL}  /* Sentinel */
};

#ifndef WITHOUT_AV
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
#endif

PyObject* VRPyCarDynamics::setType(VRPyCarDynamics* self, PyObject* args) {
    int t;
    if (!PyArg_ParseTuple(args, "i", &t)) return NULL;
    self->objPtr->setType(OSG::VRCarDynamics::TYPE(t));
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

//----------------------------------------------------------------------------------------------
PyObject* VRPyCarDynamics::geteForce(VRPyCarDynamics* self) {
    return PyFloat_FromDouble(self->objPtr->geteForce());
}

PyObject* VRPyCarDynamics::geteBreak(VRPyCarDynamics* self) {
    return PyFloat_FromDouble(self->objPtr->geteBreak());
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

PyObject* VRPyCarDynamics::setChassis(VRPyCarDynamics* self, PyObject* args) {
    VRPyTransform* dev = 0;
    int doPhys = 1;
    if (! PyArg_ParseTuple(args, "O|i", &dev, &doPhys)) return NULL;
    self->objPtr->setChassisGeo(dev->objPtr, doPhys);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::setupSimpleWheels(VRPyCarDynamics* self, PyObject* args) {
    VRPyGeometry* geo = NULL;
    float X, Zp, Zn, h, r, w, ms;
    if (! PyArg_ParseTuple(args, "Offfffff", &geo, &X, &Zp, &Zn, &h, &r, &w, &ms)) return NULL;
    self->objPtr->setupSimpleWheels(geo->objPtr, X, Zp, Zn, h, r, w, ms);
    Py_RETURN_TRUE;
}

PyObject* VRPyCarDynamics::setParameter(VRPyCarDynamics* self, PyObject* args) {
    float m, e, b;
    PyObject* mOffset = 0;
    if (! PyArg_ParseTuple(args, "fff|O", &m, &e, &b, &mOffset)) return NULL;
	self->objPtr->setParameter(m,e,b,parseVec3dList(mOffset));
	Py_RETURN_TRUE;
}


PyMethodDef VRPyDriver::methods[] = {
    {"setCar", (PyCFunction)VRPyDriver::setCar, METH_VARARGS, "Set car - setCar( car )" },
    {"followPath", (PyCFunction)VRPyDriver::followPath, METH_VARARGS, "Start the pilot to follow a path with a certain speed curve - followPath( path p, path v, | float to )" },
    {"stop", PyWrap( Driver, stop, "Stop driving", void ) },
    {"resume", PyWrap( Driver, resume, "Resume driving", void ) },
    {"setTargetSpeed", PyWrap( Driver, setTargetSpeed, "Set default target speed", void, float ) },
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
    float t = 1;
    if (! PyArg_ParseTuple(args, "OO|f", &p, &v, &t)) return NULL;
    self->objPtr->followPath(p->objPtr, v->objPtr, t);
    Py_RETURN_TRUE;
}





