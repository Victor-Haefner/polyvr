#ifndef VRPYWORLDGENERATOR_H_INCLUDED
#define VRPYWORLDGENERATOR_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "roads/VRAsphalt.h"
#include "roads/VRRoadNetwork.h"

struct VRPyAsphalt : VRPyBaseT<OSG::VRAsphalt> {
    static PyMethodDef methods[];

    static PyObject* addMarking(VRPyAsphalt* self, PyObject *args);
    static PyObject* addTrack(VRPyAsphalt* self, PyObject *args);
    static PyObject* updateTexture(VRPyAsphalt* self);
    static PyObject* clearTexture(VRPyAsphalt* self);
};

struct VRPyRoadNetwork : VRPyBaseT<OSG::VRRoadNetwork> {
    static PyMethodDef methods[];
};

#endif // VRPYWORLDGENERATOR_H_INCLUDED
