#ifndef VRPYSKY_H_INCLUDED
#define VRPYSKY_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "core/objects/geometry/VRSky.h"

struct VRPySky : VRPyBaseT<OSG::VRSky> {
    static PyMethodDef methods[];

    static PyObject* setDateTime(VRPySky* self, PyObject* args);
    static PyObject* setWeather(VRPySky* self, PyObject* args);
    static PyObject* setPosition(VRPySky* self, PyObject* args);
    static PyObject* setSpeed(VRPySky* self, PyObject* args);
};


#endif // VRPYSKY_H_INCLUDED
