#ifndef VRPYTRAFFICSIMULATION_H_INCLUDED
#define VRPYTRAFFICSIMULATION_H_INCLUDED


#include "core/scripting/VRPyBase.h"
#include "VRTrafficSimulation.h"

struct VRPyTrafficSimulation : VRPyBaseT<OSG::VRTrafficSimulation> {
    static PyMethodDef methods[];
};

#include "TrafficSimulation.h"

struct VRPyOldTrafficSimulation : VRPyBaseT<OSG::OldTrafficSimulation> {
    static PyMethodDef methods[];

    static PyObject* init(VRPyOldTrafficSimulation* self, PyObject* args);

    static PyObject* setDrawingDistance(VRPyOldTrafficSimulation* self, PyObject* args);
    static PyObject* setTrafficDensity(VRPyOldTrafficSimulation* self, PyObject* args);
    static PyObject* addVehicleType(VRPyOldTrafficSimulation* self, PyObject* args);
    static PyObject* addDriverType(VRPyOldTrafficSimulation* self, PyObject* args);
    static PyObject* setSimulationSpeed(VRPyOldTrafficSimulation* self, PyObject* args);

    static PyObject* start(VRPyOldTrafficSimulation* self, PyObject* args);
    static PyObject* pause(VRPyOldTrafficSimulation* self, PyObject* args);
    static PyObject* isRunning(VRPyOldTrafficSimulation* self, PyObject* args);

    static PyObject* setVehiclePosition(VRPyOldTrafficSimulation* self, PyObject* args);
    static PyObject* setPlayerTransform(VRPyOldTrafficSimulation* self, PyObject* args);
    static PyObject* tick(VRPyOldTrafficSimulation* self, PyObject* args);
};

#endif // VRPYTRAFFICSIMULATION_H_INCLUDED
