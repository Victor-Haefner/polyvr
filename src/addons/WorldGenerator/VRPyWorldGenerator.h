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

    static PyObject* setOntology(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* addNode(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* addLane(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* addWay(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* addRoad(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* addPath(VRPyRoadNetwork* self, PyObject *args);

    static PyObject* computeIntersectionLanes(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* computeLanePaths(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* computeIntersections(VRPyRoadNetwork* self);
    static PyObject* computeLanes(VRPyRoadNetwork* self);
    static PyObject* computeSurfaces(VRPyRoadNetwork* self);
    static PyObject* computeMarkings(VRPyRoadNetwork* self);
    static PyObject* compute(VRPyRoadNetwork* self);

    static PyObject* computeMarkingsRoad2(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* computeMarkingsIntersection(VRPyRoadNetwork* self, PyObject *args);

    static PyObject* createRoadGeometry(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* createIntersectionGeometry(VRPyRoadNetwork* self, PyObject *args);
    static PyObject* getRoadID(VRPyRoadNetwork* self);
    static PyObject* getMaterial(VRPyRoadNetwork* self);
    static PyObject* clear(VRPyRoadNetwork* self);
};

#endif // VRPYWORLDGENERATOR_H_INCLUDED
