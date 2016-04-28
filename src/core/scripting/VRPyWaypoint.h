#ifndef VRPYWAYPOINT_H_INCLUDED
#define VRPYWAYPOINT_H_INCLUDED

#include "VRPyBase.h"
#include "core/tools/VRWaypoint.h"

struct VRPyWaypoint : VRPyBaseT<OSG::VRWaypoint> {
    static PyMethodDef methods[];

    static PyObject* set(VRPyWaypoint* self, PyObject* args);
    static PyObject* get(VRPyWaypoint* self);
    static PyObject* apply(VRPyWaypoint* self, PyObject* args);
    static PyObject* setFloorPlane(VRPyWaypoint* self, PyObject* args);
    static PyObject* setSize(VRPyWaypoint* self, PyObject* args);
};

#endif // VRPYWAYPOINT_H_INCLUDED
