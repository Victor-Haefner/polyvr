#include "VRPyTrafficSimulation.h"
#include "../VRPyRealWorld.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyBaseT.h"
#include "../RealWorld.h"

using namespace OSG;

simplePyType(TrafficSimulation, New_toZero);

PyMethodDef VRPyTrafficSimulation::methods[] = {
    {"init", (PyCFunction)VRPyTrafficSimulation::init, METH_VARARGS, "Gets simulator handle" },
    {"setDrawingDistance", (PyCFunction)VRPyTrafficSimulation::setDrawingDistance, METH_VARARGS, "Sets the drawing distance" },
    {"setTrafficDensity", (PyCFunction)VRPyTrafficSimulation::setTrafficDensity, METH_VARARGS, "Sets the density of traffic" },
    {"setSimulationSpeed", (PyCFunction)VRPyTrafficSimulation::setSimulationSpeed, METH_VARARGS, "Sets the speed of the simulation" },
    {"addVehicleType", (PyCFunction)VRPyTrafficSimulation::addVehicleType, METH_VARARGS, "Adds a vehicle type" },
    {"addDriverType", (PyCFunction)VRPyTrafficSimulation::addDriverType, METH_VARARGS, "Adds a driver type" },
    {"start", (PyCFunction)VRPyTrafficSimulation::start, METH_VARARGS, "Start simulation" },
    {"pause", (PyCFunction)VRPyTrafficSimulation::pause, METH_VARARGS, "Pause simulation" },
    {"isRunning", (PyCFunction)VRPyTrafficSimulation::isRunning, METH_VARARGS, "Returns if simulation is running" },
    {"setServer", (PyCFunction)VRPyTrafficSimulation::setServer, METH_VARARGS, "Sets the server name && port" },
    {"setVehiclePosition", (PyCFunction)VRPyTrafficSimulation::setVehiclePosition, METH_VARARGS, "Sets the position of a vehicle" },
    {"setPlayerTransform", (PyCFunction)VRPyTrafficSimulation::setPlayerTransform, METH_VARARGS, "Sets the player-vehicle" },
    {"tick", (PyCFunction)VRPyTrafficSimulation::tick, METH_VARARGS, "Ticks" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyTrafficSimulation::init(VRPyTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    VRPyRealWorld* world = NULL;
    if (! PyArg_ParseTuple(args, "O", &world)) return NULL;
    if (world == NULL) { PyErr_SetString(err, "Missing world parameter"); return NULL; }
    if (!world->objPtr) { PyErr_SetString(err, "VRPyTrafficSimulation::init, world is not valid"); return NULL; }
    self->objPtr = std::shared_ptr<TrafficSimulation>(world->objPtr->getTrafficSimulation());
    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::setDrawingDistance(VRPyTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->setDrawingDistance( parseFloat(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::setTrafficDensity(VRPyTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->setTrafficDensity( parseFloat(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::addVehicleType(VRPyTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;

    int id;
    float prob, radius, speed, acc, rot;
    VRPyGeometry *geo;
    if (! PyArg_ParseTuple(args, "ifffffO", &id, &prob, &radius, &speed, &acc, &rot, &geo)) return NULL;

    if (id < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addVehicleType, id is <0"); return NULL; }
    if (prob < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addVehicleType, prob is <0"); return NULL; }
    if (radius < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addVehicleType, collisionRadius is <0"); return NULL; }
    if (speed < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addVehicleType, maxSpeed is <0"); return NULL; }
    if (acc < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addVehicleType, maxAcceleration is <0"); return NULL; }
    if (rot < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addVehicleType, maxRoration is <0"); return NULL; }
    if (geo == NULL) { PyErr_SetString(err, "VRPyTrafficSimulation::addVehicleType, no geometry has been given"); return NULL; }

    self->objPtr->addVehicleType(id, prob, radius, speed, acc, rot, static_pointer_cast<VRGeometry>(geo->objPtr));

    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::addDriverType(VRPyTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    int id;
    float prob, law, caut;
    if (! PyArg_ParseTuple(args, "ifff", &id, &prob, &law, &caut)) return NULL;

    if (id < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addDriverType, id is <0"); return NULL; }
    if (prob < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addDriverType, prob is <0"); return NULL; }
    if (law < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addDriverType, lawlessness is not in [0, 1]"); return NULL; }
    if (caut < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addDriverType, cautiousness is not in [0, 1]"); return NULL; }

    self->objPtr->addDriverType(id, prob,  law, caut);

    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::setSimulationSpeed(VRPyTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;

    float speed = parseFloat(args);
    if (speed <= 0) { PyErr_SetString(err, "VRPyTrafficSimulation::setSimulationSpeed, speed factor is to low"); return NULL; }

    self->objPtr->setSimulationSpeed( speed );

    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::start(VRPyTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->start();
    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::pause(VRPyTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->pause();
    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::isRunning(VRPyTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;

    if (self->objPtr->isRunning()) Py_RETURN_TRUE;
    else Py_RETURN_FALSE;
}

PyObject* VRPyTrafficSimulation::setServer(VRPyTrafficSimulation* self, PyObject* args) {
    string host = parseString(args);
    self->objPtr->setServer(host);
    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::setVehiclePosition(VRPyTrafficSimulation* self, PyObject* args) {
    int id;
    VRPyTransform *child  = NULL;
    VRPyTransform *child2 = NULL;
    if (! PyArg_ParseTuple(args, "iOO", &id, &child, &child2)) return NULL;
    if (child == NULL) { PyErr_SetString(err, "Missing child parameter"); return NULL; }
    if (child2 == NULL) { PyErr_SetString(err, "Missing child parameter"); return NULL; }
    if (child->objPtr == 0) { PyErr_SetString(err, "VRPyTrafficSimulation::setVehiclePosition, obj is invalid"); return NULL; }
    if (child2->objPtr == 0) { PyErr_SetString(err, "VRPyTrafficSimulation::setVehiclePosition, obj is invalid"); return NULL; }
    if (id < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::setVehiclePosition, id is <0"); return NULL; }

    self->objPtr->setVehiclePosition(id, child->objPtr->getWorldPosition(), child2->objPtr->getWorldPosition());
    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::setPlayerTransform(VRPyTrafficSimulation* self, PyObject* args) {
    VRPyTransform *child  = NULL;
    if (! PyArg_ParseTuple(args, "O", &child)) return NULL;
    if (child == NULL) { PyErr_SetString(err, "Missing child parameter"); return NULL; }
    self->objPtr->setPlayerTransform(child->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::tick(VRPyTrafficSimulation* self, PyObject* args) {
    self->objPtr->tick();
    Py_RETURN_TRUE;
}
