#ifndef VRPYWORLDGENERATOR_H_INCLUDED
#define VRPYWORLDGENERATOR_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRWorldGenerator.h"
#include "roads/VRAsphalt.h"
#include "roads/VRRoadNetwork.h"
#include "roads/VRRoad.h"

struct VRPyWorldGenerator : VRPyBaseT<OSG::VRWorldGenerator> {
    static PyMethodDef methods[];
};

struct VRPyRoadBase : VRPyBaseT<OSG::VRRoadBase> {
    static PyMethodDef methods[];
};

struct VRPyRoad : VRPyBaseT<OSG::VRRoad> {
    static PyMethodDef methods[];
};

struct VRPyAsphalt : VRPyBaseT<OSG::VRAsphalt> {
    static PyMethodDef methods[];

    static PyObject* addMarking(VRPyAsphalt* self, PyObject *args);
    static PyObject* addTrack(VRPyAsphalt* self, PyObject *args);
    static PyObject* updateTexture(VRPyAsphalt* self);
    static PyObject* clearTexture(VRPyAsphalt* self);
};

struct VRPyRoadNetwork : VRPyBaseT<OSG::VRRoadNetwork> {
    static PyMethodDef methods[];

    static PyObject* setNatureManager(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* computeGreenBelts(VRPyRoadNetwork* self);

    static PyObject* addNode(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* addGreenBelt(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* addPath(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* addArrows(VRPyRoadNetwork* self, PyObject *args);

    static PyObject* computeLanePaths(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* computeIntersections(VRPyRoadNetwork* self);
    static PyObject* computeLanes(VRPyRoadNetwork* self);
    static PyObject* computeSurfaces(VRPyRoadNetwork* self);
    static PyObject* computeMarkings(VRPyRoadNetwork* self);
    static PyObject* compute(VRPyRoadNetwork* self);

    static PyObject* getRoadID(VRPyRoadNetwork* self);
    static PyObject* getMaterial(VRPyRoadNetwork* self);
    static PyObject* updateAsphaltTexture(VRPyRoadNetwork* self);
    static PyObject* clear(VRPyRoadNetwork* self);
};

#endif // VRPYWORLDGENERATOR_H_INCLUDED
