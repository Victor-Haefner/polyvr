#ifndef VRPYTRAFFICSIMULATION_H_INCLUDED
#define VRPYTRAFFICSIMULATION_H_INCLUDED

#include "TrafficSimulation.h"
#include "core/scripting/VRPyBase.h"

struct VRPyTrafficSimulation : VRPyBaseT<OSG::TrafficSimulation> {
    static PyMethodDef methods[];

    static PyObject* init(VRPyTrafficSimulation* self, PyObject* args);

    static PyObject* setDrawingDistance(VRPyTrafficSimulation* self, PyObject* args);
    static PyObject* setTrafficDensity(VRPyTrafficSimulation* self, PyObject* args);
    static PyObject* addVehicleType(VRPyTrafficSimulation* self, PyObject* args);
    static PyObject* addDriverType(VRPyTrafficSimulation* self, PyObject* args);
    static PyObject* setSimulationSpeed(VRPyTrafficSimulation* self, PyObject* args);

    static PyObject* start(VRPyTrafficSimulation* self, PyObject* args);
    static PyObject* pause(VRPyTrafficSimulation* self, PyObject* args);
    static PyObject* isRunning(VRPyTrafficSimulation* self, PyObject* args);
    static PyObject* setServer(VRPyTrafficSimulation* self, PyObject* args);

    static PyObject* setVehiclePosition(VRPyTrafficSimulation* self, PyObject* args);
    static PyObject* setPlayerTransform(VRPyTrafficSimulation* self, PyObject* args);
    static PyObject* tick(VRPyTrafficSimulation* self, PyObject* args);
};

#endif // VRPYTRAFFICSIMULATION_H_INCLUDED
