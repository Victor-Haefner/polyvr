#include "VRPyTrafficSimulation.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyBaseT.h"
#include "../RealWorld.h"

template<> PyTypeObject VRPyBaseT<OSG::TrafficSimulation>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.TrafficSimulation",             /*tp_name*/
    sizeof(VRPyTrafficSimulation),             /*tp_basicsize*/
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
    "VRPyTrafficSimulation binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyTrafficSimulation::methods,             /* tp_methods */
    VRPyTrafficSimulation::members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_toZero,                 /* tp_new */
};

PyMemberDef VRPyTrafficSimulation::members[] = {
    {NULL}  /* Sentinel */
};

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

    if (self->obj != 0) { PyErr_SetString(err, "VRPyTrafficSimulation::init, has already been initialized"); return NULL; }

    PyObject* child = NULL;
    if (! PyArg_ParseTuple(args, "O", &child)) return NULL;
    if (child == NULL) { PyErr_SetString(err, "Missing child parameter"); return NULL; }
    VRPyObject* _child = (VRPyObject*)child;

    if (_child->obj == 0) { PyErr_SetString(err, "VRPyTrafficSimulation::init, world is not valid"); return NULL; }

    RealWorld *world = (RealWorld*)_child->obj;

    self->obj = world->getTrafficSimulation();

    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::setDrawingDistance(VRPyTrafficSimulation* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTrafficSimulation::setDrawingDistance, obj is invalid"); return NULL; }
    self->obj->setDrawingDistance( parseFloat(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::setTrafficDensity(VRPyTrafficSimulation* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTrafficSimulation::setTrafficDensity, obj is invalid"); return NULL; }
    self->obj->setTrafficDensity( parseFloat(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::addVehicleType(VRPyTrafficSimulation* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addVehicleType, obj is invalid"); return NULL; }

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

    self->obj->addVehicleType(id, prob, radius, speed, acc, rot, static_pointer_cast<VRGeometry>(geo->objPtr));

    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::addDriverType(VRPyTrafficSimulation* self, PyObject* args) {
    int id;
    float prob, law, caut;
    if (! PyArg_ParseTuple(args, "ifff", &id, &prob, &law, &caut)) return NULL;

    if (self->obj == 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addDriverType, obj is invalid"); return NULL; }

    if (id < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addDriverType, id is <0"); return NULL; }
    if (prob < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addDriverType, prob is <0"); return NULL; }
    if (law < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addDriverType, lawlessness is not in [0, 1]"); return NULL; }
    if (caut < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::addDriverType, cautiousness is not in [0, 1]"); return NULL; }

    self->obj->addDriverType(id, prob,  law, caut);

    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::setSimulationSpeed(VRPyTrafficSimulation* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTrafficSimulation::setSimulationSpeed, obj is invalid"); return NULL; }

    float speed = parseFloat(args);
    if (speed <= 0) { PyErr_SetString(err, "VRPyTrafficSimulation::setSimulationSpeed, speed factor is to low"); return NULL; }

    self->obj->setSimulationSpeed( speed );

    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::start(VRPyTrafficSimulation* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTrafficSimulation::start, obj is invalid"); return NULL; }
    self->obj->start();
    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::pause(VRPyTrafficSimulation* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTrafficSimulation::pause, obj is invalid"); return NULL; }
    self->obj->pause();
    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::isRunning(VRPyTrafficSimulation* self, PyObject* args) {

    if (self->obj == 0) { PyErr_SetString(err, "VRPyTrafficSimulation::isRunning, obj is invalid"); return NULL; }

    if (self->obj->isRunning())
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* VRPyTrafficSimulation::setServer(VRPyTrafficSimulation* self, PyObject* args) {

    string host = parseString(args);
    self->obj->setServer(host);

    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::setVehiclePosition(VRPyTrafficSimulation* self, PyObject* args) {
    int id;
    VRPyTransform *child  = NULL;
    VRPyTransform *child2 = NULL;
    if (! PyArg_ParseTuple(args, "iOO", &id, &child, &child2)) return NULL;
    if (child == NULL) { PyErr_SetString(err, "Missing child parameter"); return NULL; }
    if (child2 == NULL) { PyErr_SetString(err, "Missing child parameter"); return NULL; }
    if (child->obj == 0) { PyErr_SetString(err, "VRPyTrafficSimulation::setVehiclePosition, obj is invalid"); return NULL; }
    if (child2->obj == 0) { PyErr_SetString(err, "VRPyTrafficSimulation::setVehiclePosition, obj is invalid"); return NULL; }
    if (id < 0) { PyErr_SetString(err, "VRPyTrafficSimulation::setVehiclePosition, id is <0"); return NULL; }

    self->obj->setVehiclePosition(id, child->obj->getWorldPosition(), child2->obj->getWorldPosition());
    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::setPlayerTransform(VRPyTrafficSimulation* self, PyObject* args) {
    VRPyTransform *child  = NULL;
    if (! PyArg_ParseTuple(args, "O", &child)) return NULL;
    if (child == NULL) { PyErr_SetString(err, "Missing child parameter"); return NULL; }
    self->obj->setPlayerTransform(child->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyTrafficSimulation::tick(VRPyTrafficSimulation* self, PyObject* args) {
    self->obj->tick();
    Py_RETURN_TRUE;
}
