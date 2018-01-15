#include "VRPyTrafficSimulation.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(TrafficSimulation, New_ptr);

PyMethodDef VRPyTrafficSimulation::methods[] = {
    {"setRoadNetwork", PyWrap( TrafficSimulation, setRoadNetwork, "Set road network", void, VRRoadNetworkPtr ) },
    {"setTraffic", PyWrap( TrafficSimulation, setTraffic, "Set overall traffic", void, float, int ) },
    {NULL}  /* Sentinel */
};


#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyTransform.h"

using namespace OSG;

simplePyType(OldTrafficSimulation, New_toZero);

PyMethodDef VRPyOldTrafficSimulation::methods[] = {
    {"init", (PyCFunction)VRPyOldTrafficSimulation::init, METH_VARARGS, "Gets simulator handle" },
    {"setDrawingDistance", (PyCFunction)VRPyOldTrafficSimulation::setDrawingDistance, METH_VARARGS, "Sets the drawing distance" },
    {"setTrafficDensity", (PyCFunction)VRPyOldTrafficSimulation::setTrafficDensity, METH_VARARGS, "Sets the density of traffic" },
    {"setSimulationSpeed", (PyCFunction)VRPyOldTrafficSimulation::setSimulationSpeed, METH_VARARGS, "Sets the speed of the simulation" },
    {"addVehicleType", (PyCFunction)VRPyOldTrafficSimulation::addVehicleType, METH_VARARGS, "Adds a vehicle type" },
    {"addDriverType", (PyCFunction)VRPyOldTrafficSimulation::addDriverType, METH_VARARGS, "Adds a driver type" },
    {"start", (PyCFunction)VRPyOldTrafficSimulation::start, METH_VARARGS, "Start simulation" },
    {"pause", (PyCFunction)VRPyOldTrafficSimulation::pause, METH_VARARGS, "Pause simulation" },
    {"isRunning", (PyCFunction)VRPyOldTrafficSimulation::isRunning, METH_VARARGS, "Returns if simulation is running" },
    {"setVehiclePosition", (PyCFunction)VRPyOldTrafficSimulation::setVehiclePosition, METH_VARARGS, "Sets the position of a vehicle" },
    {"setPlayerTransform", (PyCFunction)VRPyOldTrafficSimulation::setPlayerTransform, METH_VARARGS, "Sets the player-vehicle" },
    {"tick", (PyCFunction)VRPyOldTrafficSimulation::tick, METH_VARARGS, "Ticks" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyOldTrafficSimulation::init(VRPyOldTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    /*VRPyRealWorld* world = NULL;
    if (! PyArg_ParseTuple(args, "O", &world)) return NULL;
    if (world == NULL) { PyErr_SetString(err, "Missing world parameter"); return NULL; }
    if (!world->objPtr) { PyErr_SetString(err, "VRPyOldTrafficSimulation::init, world is not valid"); return NULL; }
    self->objPtr = std::shared_ptr<OldTrafficSimulation>(world->objPtr->getTrafficSimulation());*/
    Py_RETURN_TRUE;
}

PyObject* VRPyOldTrafficSimulation::setDrawingDistance(VRPyOldTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->setDrawingDistance( parseFloat(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyOldTrafficSimulation::setTrafficDensity(VRPyOldTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->setTrafficDensity( parseFloat(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyOldTrafficSimulation::addVehicleType(VRPyOldTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;

    int id;
    float prob, radius, speed, acc, rot;
    VRPyGeometry *geo;
    if (! PyArg_ParseTuple(args, "ifffffO", &id, &prob, &radius, &speed, &acc, &rot, &geo)) return NULL;

    if (id < 0) { PyErr_SetString(err, "VRPyOldTrafficSimulation::addVehicleType, id is <0"); return NULL; }
    if (prob < 0) { PyErr_SetString(err, "VRPyOldTrafficSimulation::addVehicleType, prob is <0"); return NULL; }
    if (radius < 0) { PyErr_SetString(err, "VRPyOldTrafficSimulation::addVehicleType, collisionRadius is <0"); return NULL; }
    if (speed < 0) { PyErr_SetString(err, "VRPyOldTrafficSimulation::addVehicleType, maxSpeed is <0"); return NULL; }
    if (acc < 0) { PyErr_SetString(err, "VRPyOldTrafficSimulation::addVehicleType, maxAcceleration is <0"); return NULL; }
    if (rot < 0) { PyErr_SetString(err, "VRPyOldTrafficSimulation::addVehicleType, maxRoration is <0"); return NULL; }
    if (geo == NULL) { PyErr_SetString(err, "VRPyOldTrafficSimulation::addVehicleType, no geometry has been given"); return NULL; }

    self->objPtr->addVehicleType(id, prob, radius, speed, acc, rot, static_pointer_cast<VRGeometry>(geo->objPtr));

    Py_RETURN_TRUE;
}

PyObject* VRPyOldTrafficSimulation::addDriverType(VRPyOldTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    int id;
    float prob, law, caut;
    if (! PyArg_ParseTuple(args, "ifff", &id, &prob, &law, &caut)) return NULL;

    if (id < 0) { PyErr_SetString(err, "VRPyOldTrafficSimulation::addDriverType, id is <0"); return NULL; }
    if (prob < 0) { PyErr_SetString(err, "VRPyOldTrafficSimulation::addDriverType, prob is <0"); return NULL; }
    if (law < 0) { PyErr_SetString(err, "VRPyOldTrafficSimulation::addDriverType, lawlessness is not in [0, 1]"); return NULL; }
    if (caut < 0) { PyErr_SetString(err, "VRPyOldTrafficSimulation::addDriverType, cautiousness is not in [0, 1]"); return NULL; }

    self->objPtr->addDriverType(id, prob,  law, caut);

    Py_RETURN_TRUE;
}

PyObject* VRPyOldTrafficSimulation::setSimulationSpeed(VRPyOldTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;

    float speed = parseFloat(args);
    if (speed <= 0) { PyErr_SetString(err, "VRPyOldTrafficSimulation::setSimulationSpeed, speed factor is to low"); return NULL; }

    self->objPtr->setSimulationSpeed( speed );

    Py_RETURN_TRUE;
}

PyObject* VRPyOldTrafficSimulation::start(VRPyOldTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->start();
    Py_RETURN_TRUE;
}

PyObject* VRPyOldTrafficSimulation::pause(VRPyOldTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->pause();
    Py_RETURN_TRUE;
}

PyObject* VRPyOldTrafficSimulation::isRunning(VRPyOldTrafficSimulation* self, PyObject* args) {
	if (!self->valid()) return NULL;

    if (self->objPtr->isRunning()) Py_RETURN_TRUE;
    else Py_RETURN_FALSE;
}

PyObject* VRPyOldTrafficSimulation::setVehiclePosition(VRPyOldTrafficSimulation* self, PyObject* args) {
    int id;
    VRPyTransform *child  = NULL;
    VRPyTransform *child2 = NULL;
    if (! PyArg_ParseTuple(args, "iOO", &id, &child, &child2)) return NULL;
    if (child == NULL) { PyErr_SetString(err, "Missing child parameter"); return NULL; }
    if (child2 == NULL) { PyErr_SetString(err, "Missing child parameter"); return NULL; }
    if (child->objPtr == 0) { PyErr_SetString(err, "VRPyOldTrafficSimulation::setVehiclePosition, obj is invalid"); return NULL; }
    if (child2->objPtr == 0) { PyErr_SetString(err, "VRPyOldTrafficSimulation::setVehiclePosition, obj is invalid"); return NULL; }
    if (id < 0) { PyErr_SetString(err, "VRPyOldTrafficSimulation::setVehiclePosition, id is <0"); return NULL; }

    self->objPtr->setVehiclePosition(id, child->objPtr->getWorldPosition(), child2->objPtr->getWorldPosition());
    Py_RETURN_TRUE;
}

PyObject* VRPyOldTrafficSimulation::setPlayerTransform(VRPyOldTrafficSimulation* self, PyObject* args) {
    VRPyTransform *child  = NULL;
    if (! PyArg_ParseTuple(args, "O", &child)) return NULL;
    if (child == NULL) { PyErr_SetString(err, "Missing child parameter"); return NULL; }
    self->objPtr->setPlayerTransform(child->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyOldTrafficSimulation::tick(VRPyOldTrafficSimulation* self, PyObject* args) {
    self->objPtr->tick();
    Py_RETURN_TRUE;
}
