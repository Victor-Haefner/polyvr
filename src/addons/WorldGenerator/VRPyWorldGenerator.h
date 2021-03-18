#ifndef VRPYWORLDGENERATOR_H_INCLUDED
#define VRPYWORLDGENERATOR_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRWorldGenerator.h"
#include "GIS/OSMMap.h"
#include "GIS/VRAtlas.h"
#include "roads/VRAsphalt.h"
#include "roads/VRRoadNetwork.h"
#include "roads/VRRoad.h"
#include "roads/VRRoadIntersection.h"
#include "roads/VRTrafficSigns.h"
#include "buildings/VRDistrict.h"

struct VRPyWorldGenerator : VRPyBaseT<OSG::VRWorldGenerator> {
    static PyMethodDef methods[];
};

struct VRPyRoadBase : VRPyBaseT<OSG::VRRoadBase> {
    static PyMethodDef methods[];
};

struct VRPyRoad : VRPyBaseT<OSG::VRRoad> {
    static PyMethodDef methods[];
};

struct VRPyTrafficSigns : VRPyBaseT<OSG::VRTrafficSigns> {
    static PyMethodDef methods[];
};

struct VRPyRoadIntersection : VRPyBaseT<OSG::VRRoadIntersection> {
    static PyMethodDef methods[];
};

struct VRPyAsphalt : VRPyBaseT<OSG::VRAsphalt> {
    static PyMethodDef methods[];
};

struct VRPyRoadNetwork : VRPyBaseT<OSG::VRRoadNetwork> {
    static PyMethodDef methods[];
};

struct VRPyDistrict : VRPyBaseT<OSG::VRDistrict> {
    static PyMethodDef methods[];
};

struct VRPyOSMMap : VRPyBaseT<OSG::OSMMap> {
    static PyMethodDef methods[];
};

struct VRPyOSMRelation : VRPyBaseT<OSG::OSMRelation> {
    static PyMethodDef methods[];
};

struct VRPyOSMWay : VRPyBaseT<OSG::OSMWay> {
    static PyMethodDef methods[];
};

struct VRPyOSMNode : VRPyBaseT<OSG::OSMNode> {
    static PyMethodDef methods[];
};

struct VRPyOSMBase : VRPyBaseT<OSG::OSMBase> {
    static PyMethodDef methods[];
};

struct VRPyAtlas : VRPyBaseT<OSG::VRAtlas> {
    static PyMethodDef methods[];
};

#endif // VRPYWORLDGENERATOR_H_INCLUDED
