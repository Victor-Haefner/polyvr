#include "VRPyWorldGenerator.h"
#include "addons/WorldGenerator/nature/VRPyNature.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyPath.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyPolygon.h"
#include "addons/Semantics/Reasoning/VRPyOntology.h"
#include "core/scripting/VRPyBaseFactory.h"

using namespace OSG;

template<> PyObject* VRPyTypeCaster::cast(const VRRoadPtr& e) { return VRPyRoad::fromSharedPtr(e); }
template<> PyObject* VRPyTypeCaster::cast(const VRRoadNetworkPtr& e) { return VRPyRoadNetwork::fromSharedPtr(e); }
template<> PyObject* VRPyTypeCaster::cast(const VRWorldGeneratorPtr& e) { return VRPyWorldGenerator::fromSharedPtr(e); }
template<> PyObject* VRPyTypeCaster::cast(const VRAsphaltPtr& e) { return VRPyAsphalt::fromSharedPtr(e); }

simpleVRPyType(WorldGenerator, New_ptr );
simpleVRPyType(RoadBase, 0);
simpleVRPyType(Road, New_ptr);
simpleVRPyType(RoadNetwork, New_ptr);
simpleVRPyType(Asphalt, New_ptr);

PyMethodDef VRPyWorldGenerator::methods[] = {
    {"addAsset", PyWrap( WorldGenerator, addAsset, "Add an asset template", void, string, VRTransformPtr ) },
    {"getAssetManager", PyWrap( WorldGenerator, getAssetManager, "Get the asset manager", VRObjectManagerPtr ) },
    {"getRoadNetwork", PyWrap( WorldGenerator, getRoadNetwork, "Access road network", VRRoadNetworkPtr ) },
    {"getNature", PyWrap( WorldGenerator, getNature, "Access nature module", VRNaturePtr ) },
    {"getTerrain", PyWrap( WorldGenerator, getTerrain, "Access the terrain", VRTerrainPtr ) },
    {"setOntology", PyWrap( WorldGenerator, setOntology, "Set ontology", void, VROntologyPtr ) },
    {"addMaterial", PyWrap( WorldGenerator, addMaterial, "Add a named material", void, string, VRMaterialPtr ) },
    {"getMaterial", PyWrap( WorldGenerator, getMaterial, "Get a material by name", VRMaterialPtr, string ) },
    {"addOSMMap", PyWrapOpt( WorldGenerator, addOSMMap, "Add an OpenStreetMap map", "-1|-1|-1", void, string, double, double, double ) },
    {"reloadOSMMap", PyWrapOpt( WorldGenerator, reloadOSMMap, "Reload OSM data", "-1|-1|-1", void, double, double, double ) },
    {"clear", PyWrap( WorldGenerator, clear, "Clear everything", void ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRoadBase::methods[] = {
    {"addLane", PyWrapOpt( RoadBase, addLane, "Add a lane", "0", VREntityPtr, int, float, bool ) },
    {"addNode", PyWrapOpt( RoadBase, addNode, "Add a node", "1|0", VREntityPtr, int, Vec3d, bool, float ) },
    {"addPath", PyWrap( RoadBase, addPath, "Add a new path", VREntityPtr, string, string, vector<VREntityPtr>, vector<Vec3d> ) },
    {"addArrows", PyWrap( RoadBase, addArrows, "Add a new path", VREntityPtr, VREntityPtr, float, vector<float> ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRoad::methods[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyAsphalt::methods[] = {
    {"addMarking", PyWrapOpt(Asphalt, addMarking, "Add marking", "0|0", void, int, pathPtr, float, int, float ) },
    {"addTrack", PyWrapOpt(Asphalt, addTrack, "Add track", "0|0", void, int, pathPtr, float, int, float ) },
    {"clearTexture", PyWrap(Asphalt, clearTexture, "Clear internal textures", void ) },
    {"updateTexture", PyWrap(Asphalt, updateTexture, "Update internal textures", void ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRoadNetwork::methods[] = {
    {"addGreenBelt", PyWrap( RoadNetwork, addGreenBelt, "Add a green lane", VREntityPtr, VREntityPtr, float ) },
    {"addWay", PyWrap( RoadNetwork, addWay, "Add a way", VRRoadPtr, string, vector<VREntityPtr>, int, string ) },
    {"addRoad", PyWrap( RoadNetwork, addRoad, "Add a road", VRRoadPtr, string, string, VREntityPtr, VREntityPtr, Vec3d, Vec3d, int ) },
    {"addKirb", PyWrap( RoadNetwork, addKirb, "Add a kirb, polygon, texture", void, VRPolygonPtr, float ) },
    {"computeLanePaths", PyWrap( RoadNetwork, computeLanePaths, "Compute the path of each lane of a road", void, VREntityPtr ) },
    {"computeIntersections", PyWrap( RoadNetwork, computeIntersections, "Compute the intersections", void ) },
    {"computeLanes", PyWrap( RoadNetwork, computeLanes, "Compute the lanes", void ) },
    {"computeSurfaces", PyWrap( RoadNetwork, computeSurfaces, "Compute the surfaces" , void )},
    {"computeMarkings", PyWrap( RoadNetwork, computeMarkings, "Compute the markings", void ) },
    {"compute", PyWrap( RoadNetwork, compute, "Compute everything", void ) },
    {"getGraph", PyWrap( RoadNetwork, getGraph, "Get a road ID", GraphPtr ) },
    {"getGraphEdgeDirections", PyWrap( RoadNetwork, getGraphEdgeDirections, "Get a road ID", vector<Vec3d>, int ) },
    {"getRoadID", PyWrap( RoadNetwork, getRoadID, "Get a road ID", int ) },
    {"getMaterial", PyWrap( RoadNetwork, getMaterial, "Get road material", VRAsphaltPtr ) },
    {"updateAsphaltTexture", PyWrap( RoadNetwork, updateAsphaltTexture, "Update markings and tracks on asphalt texture", void ) },
    {"computeGreenBelts", PyWrap( RoadNetwork, computeGreenBelts, "Compute green belt areas", vector<VRPolygonPtr> ) },
    {"clear", PyWrap( RoadNetwork, clear, "Clear all data", void ) },
    {NULL}  /* Sentinel */
};



