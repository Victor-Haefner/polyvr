#include "VRPyWorldGenerator.h"
#include "addons/WorldGenerator/nature/VRPyNature.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyPath.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyPolygon.h"
#include "core/objects/material/VRTextureMosaic.h"
#include "addons/Semantics/Reasoning/VRPyOntology.h"
#include "core/scripting/VRPyBaseFactory.h"

#include <OpenSG/OSGVector.h>

using namespace OSG;

template<> PyObject* VRPyTypeCaster::cast(const VRWorldGenerator::OsmEntity& e) {
    PyObject* epy = PyTuple_New(2);
    PyTuple_SetItem(epy, 0, VRPyTypeCaster::cast(e.pnts));
    PyTuple_SetItem(epy, 1, VRPyTypeCaster::cast(e.tags));
    return epy;
}

template<> bool toValue(PyObject* o, VRWorldGenerator::VRUserGenCbPtr& v) {
    //if (!VRPyEntity::check(o)) return 0; // TODO: add checks!
    if (VRPyBase::isNone(o)) v = 0;
    else {
        Py_IncRef(o);
        v = VRWorldGenerator::VRUserGenCb::create( "pyExecCall", bind(VRPyBase::execPyCall<VRWorldGenerator::OsmEntity, bool>, o, PyTuple_New(1), placeholders::_1) );
    }
    return 1;
}

simpleVRPyType(WorldGenerator, New_ptr );
simpleVRPyType(RoadBase, 0);
simpleVRPyType(Road, New_ptr);
simpleVRPyType(RoadIntersection, New_ptr);
simpleVRPyType(RoadNetwork, New_ptr);
simpleVRPyType(District, New_ptr);
simpleVRPyType(Asphalt, New_ptr);
simpleVRPyType(TrafficSigns, New_ptr);

simpleVRPyType(MapManager, New_ptr);

simplePyType(OSMMap, New_ptr);
simplePyType(OSMRelation, 0);
simplePyType(OSMWay, 0);
simplePyType(OSMNode, 0);
simplePyType(OSMBase, 0);

PyMethodDef VRPyWorldGenerator::methods[] = {
    {"addAsset", PyWrap( WorldGenerator, addAsset, "Add an asset template", void, string, VRTransformPtr ) },
    {"getAssetManager", PyWrap( WorldGenerator, getAssetManager, "Get the asset manager", VRObjectManagerPtr ) },
    {"getRoadNetwork", PyWrap( WorldGenerator, getRoadNetwork, "Access road network", VRRoadNetworkPtr ) },
    {"getTrafficSigns", PyWrap( WorldGenerator, getTrafficSigns, "Access traffic signs", VRTrafficSignsPtr ) },
    {"getNature", PyWrap( WorldGenerator, getNature, "Access nature module", VRNaturePtr ) },
    {"getTerrain", PyWrap( WorldGenerator, getTerrain, "Access the terrain", VRTerrainPtr ) },
    {"getDistrict", PyWrap( WorldGenerator, getDistrict, "Access the district module", VRDistrictPtr ) },
    {"getLodTree", PyWrap( WorldGenerator, getLodTree, "Access the lod tree", VRLodTreePtr ) },
    {"setOntology", PyWrap( WorldGenerator, setOntology, "Set ontology", void, VROntologyPtr ) },
    {"addMaterial", PyWrap( WorldGenerator, addMaterial, "Add a named material", void, string, VRMaterialPtr ) },
    {"getMaterial", PyWrap( WorldGenerator, getMaterial, "Get a material by name", VRMaterialPtr, string ) },
    {"getMiscArea", PyWrap( WorldGenerator, getMiscArea, "Get the Geometry of a misc area by Entity", VRGeometryPtr, VREntityPtr ) },
    {"addOSMMap", PyWrapOpt( WorldGenerator, addOSMMap, "Add an OpenStreetMap map: path to OSM map, opt: double subN, double subE, double subSize, -1|-1|-1", "-1|-1|-1", void, string, double, double, double ) },
    {"addGML", PyWrapOpt( WorldGenerator, addGML, "adding a GML file and generate buildings, input: file to path, EPSG Code", "31467", void, string, int) },
    {"setupLODTerrain", PyWrapOpt( WorldGenerator, setupLODTerrain, "Sets up LOD for terrain: path to heightmap, path to texture (opt, default = ""), scale (opt, default = 1.0), cache (opt, default = True)", "|1.0|1|1|1 1 1 1|0", void, string, string, float, bool, bool, Color4f, float ) },
    {"readOSMMap", PyWrap( WorldGenerator, readOSMMap, "Read OpenStreetMap map without adding", void, string ) },
    {"getOSMMap", PyWrap( WorldGenerator, getOSMMap, "Access OSM map", OSMMapPtr ) },
    {"getGMLMap", PyWrap( WorldGenerator, getGMLMap, "Access GML map", OSMMapPtr ) },
    {"reloadOSMMap", PyWrapOpt( WorldGenerator, reloadOSMMap, "Reload OSM data", "-1|-1|-1", void, double, double, double ) },
    {"clear", PyWrap( WorldGenerator, clear, "Clear everything", void ) },
    {"getStats", PyWrap( WorldGenerator, getStats, "Return stats as string", string ) },
    {"setupPhysics", PyWrap( WorldGenerator, setupPhysics, "Process collision shapes", void ) },
    {"updatePhysics", PyWrap( WorldGenerator, updatePhysics, "Update physicalized region", void, Boundingbox ) },
#ifndef WITHOUT_BULLET
    {"getPhysicsSystem", PyWrap( WorldGenerator, getPhysicsSystem, "Return dynamic collision manager", VRSpatialCollisionManagerPtr ) },
#endif
    {"setUserCallback", PyWrap( WorldGenerator, setUserCallback, "Setup user callback", void, VRWorldGenerator::VRUserGenCbPtr ) },
    {NULL}  /* Sentinel */
};

typedef map<string, OSMRelationPtr> osmRelationMap;
typedef map<string, OSMWayPtr> osmWayMap;
typedef map<string, OSMNodePtr> osmNodeMap;
typedef map<string, string> osmTagMap;

PyMethodDef VRPyOSMMap::methods[] = {
    {"readFile", PyWrap2( OSMMap, readFile, "readFile ", void, string ) },
    {"readGEOJSON", PyWrap2( OSMMap, readGEOJSON, "reads a GEOJSON file and makes a readable OSM object", void, string ) },
    //{"readSHAPE", PyWrap2( OSMMap, readSHAPE, "reads a SHAPE file and makes a readable OSM object", void, string ) },
    {"readGML", PyWrapOpt2( OSMMap, readGML, "reads a GML file and makes a readable OSM object - input: path to file, EPSG Code", "31467",void, string, int ) },
    {"convertGKtoLatLon", PyWrap2( OSMMap, convertGKtoLatLon, "convert Gauß Krüger lat lon - input: northing, easting, EPSG Code", Vec2d, double, double, int) },
    {"writeFile", PyWrap2( OSMMap, writeFile, "writeFile ", void, string ) },
    {"filterFileStreaming", PyWrap2( OSMMap, filterFileStreaming, "filter OSM file with whitelist via stream - input path, whitelist", void, string, vector<vector<string>> ) },
    {"readFileStreaming", PyWrap2( OSMMap, readFileStreaming, "reads OSM file via stream, builds map ", int, string ) },
    {"getRelations", PyWrap2( OSMMap, getRelations, "Access OSM relations", osmRelationMap ) },
    {"getWays", PyWrap2( OSMMap, getWays, "Access OSM ways", osmWayMap ) },
    {"getNodes", PyWrap2( OSMMap, getNodes, "Access OSM nodes", osmNodeMap ) },
    {"getWay", PyWrap2( OSMMap, getWay, "Access OSM way", OSMWayPtr, string ) },
    {"getNode", PyWrap2( OSMMap, getNode, "Access OSM node", OSMNodePtr, string ) },
    {"subArea", PyWrap2( OSMMap, subArea, "Return map of subarea (latMin, latMax, lonMin, lonMax)", OSMMapPtr, double, double, double, double ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyOSMRelation::methods[] = {
    {"toString", PyWrap2( OSMRelation, toString, "As string", string ) },
    {"getNodes", PyWrap2( OSMRelation, getNodes, "Access nodes", vector<string> ) },
    {"getWays", PyWrap2( OSMRelation, getWays, "Access ways", vector<string> ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyOSMWay::methods[] = {
    {"getPolygon", PyWrap2( OSMWay, getPolygon, "Access polygon", VRPolygon ) },
    {"toString", PyWrap2( OSMWay, toString, "As string", string ) },
    {"getNodes", PyWrapOpt2( OSMWay, getNodes, "Access nodes", "1", vector<string>, float ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyOSMNode::methods[] = {
    {"toString", PyWrap2( OSMNode, toString, "As string", string ) },
    {"getPosition", PyWrap2( OSMNode, getPosition, "Access position - lat, lon", Vec2d ) },
    {"getPosition3", PyWrap2( OSMNode, getPosition3, "Access position - lat, lon, elevation", Vec3d ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyOSMBase::methods[] = {
    {"getID", PyWrap2( OSMBase, getID, "Access ID", string ) },
    {"getTags", PyWrap2( OSMBase, getTags, "Access tags", osmTagMap ) },
    {"toString", PyWrap2( OSMBase, toString, "As string", string ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRoadBase::methods[] = {
    {"addLane", PyWrapOpt( RoadBase, addLane, "Add a lane", "0", VREntityPtr, int, float, bool ) },
    {"addNode", PyWrapOpt( RoadBase, addNode, "Add a node", "1|0", VREntityPtr, int, Vec3d, bool, float ) },
    {"addPath", PyWrap( RoadBase, addPath, "Add a new path", VREntityPtr, string, string, vector<VREntityPtr>, vector<Vec3d> ) },
    {"addArrows", PyWrapOpt( RoadBase, addArrows, "Add a new path", "0", VREntityPtr, VREntityPtr, float, vector<float>, int ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRoad::methods[] = {
    {"addTrafficLight", PyWrap( Road, addTrafficLight, "Add a traffic light", void, Vec3d ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRoadIntersection::methods[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyDistrict::methods[] = {
    {"remBuilding", PyWrap( District, remBuilding, "Remove a building by address", void, string, string ) },
    {"addBuilding", PyWrapOpt( District, addBuilding, "Add a building, outline, stories, housenumber, streetname", "||resitential", void, VRPolygonPtr, int, string, string, string ) },
    {"addTexture", PyWrap( District, addTexture, "Add a texture for the building, texture, type, the type can be roof, wall, window or door", void, VRTexturePtr, string ) },
    {"addTextures", PyWrap( District, addTextures, "Add a folder with textures, folder, type, for the types see addTexture", void, string, string ) },
    {"getTexture", PyWrap( District, getTexture, "Get mega texture", VRTextureMosaicPtr ) },
    {"clear", PyWrap( District, clear, "Clear all buildings", void ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyAsphalt::methods[] = {
    {"addMarking", PyWrapOpt(Asphalt, addMarking, "Add marking", "0|0|1", void, int, PathPtr, float, float, float, int ) },
    {"addTrack", PyWrapOpt(Asphalt, addTrack, "Add track", "0|0", void, int, PathPtr, float, float, float ) },
    {"clearTexture", PyWrap(Asphalt, clearTexture, "Clear internal textures", void ) },
    {"updateTexture", PyWrap(Asphalt, updateTexture, "Update internal textures", void ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRoadNetwork::methods[] = {
    {"addGreenBelt", PyWrap( RoadNetwork, addGreenBelt, "Add a green lane", VREntityPtr, VREntityPtr, float ) },
    {"addWay", PyWrap( RoadNetwork, addWay, "Add a way", VRRoadPtr, string, vector<VREntityPtr>, int, string ) },
    {"addRoad", PyWrap( RoadNetwork, addRoad, "Add a road", VRRoadPtr, string, string, VREntityPtr, VREntityPtr, Vec3d, Vec3d, int ) },
    {"addLongRoad", PyWrap( RoadNetwork, addLongRoad, "Add a long road", VRRoadPtr, string, string, vector<VREntityPtr>, vector<Vec3d>, int ) },
    {"addKirb", PyWrap( RoadNetwork, addKirb, "Add a kirb, polygon, texture", void, VRPolygonPtr, float ) },
    {"computeLanePaths", PyWrap( RoadNetwork, computeLanePaths, "Compute the path of each lane of a road", void, VREntityPtr ) },
    {"computeIntersections", PyWrap( RoadNetwork, computeIntersections, "Compute the intersections", void ) },
    {"computeLanes", PyWrap( RoadNetwork, computeLanes, "Compute the lanes", void ) },
    {"computeSurfaces", PyWrap( RoadNetwork, computeSurfaces, "Compute the surfaces" , void )},
    {"computeMarkings", PyWrap( RoadNetwork, computeMarkings, "Compute the markings", void ) },
    {"compute", PyWrap( RoadNetwork, compute, "Compute everything", void ) },
    {"getGraph", PyWrap( RoadNetwork, getGraph, "Get a road ID", GraphPtr ) },
    {"toggleGraph", PyWrap( RoadNetwork, toggleGraph, "enables/disables visualisation of: roadnetwork graph", void ) },
    {"getGraphEdgeDirections", PyWrap( RoadNetwork, getGraphEdgeDirections, "Get a road ID", vector<Vec3d>, int ) },
    {"getRoadID", PyWrap( RoadNetwork, getRoadID, "Get a road ID", int ) },
    {"getMaterial", PyWrap( RoadNetwork, getMaterial, "Get road material", VRAsphaltPtr ) },
    {"getArrowMaterial", PyWrap( RoadNetwork, getArrowMaterial, "Get road arrows material", VRAsphaltPtr ) },
    {"updateAsphaltTexture", PyWrap( RoadNetwork, updateAsphaltTexture, "Update markings and tracks on asphalt texture", void ) },
    {"computeGreenBelts", PyWrap( RoadNetwork, computeGreenBelts, "Compute green belt areas", vector<VRPolygonPtr> ) },
    {"clear", PyWrap( RoadNetwork, clear, "Clear all data", void ) },
    {"getRoadByName", PyWrap( RoadNetwork, getRoadByName, "Get road by street name", vector<VRRoadPtr>, string ) },
    {"getRoads", PyWrap( RoadNetwork, getRoads, "Return all roads", vector<VRRoadPtr> ) },
    {"getIntersections", PyWrap( RoadNetwork, getIntersections, "Return all intersections", vector<VRRoadIntersectionPtr> ) },
    {"getPreviousRoads", PyWrap( RoadNetwork, getPreviousRoads, "Get the previous roads", vector<VREntityPtr>, VREntityPtr ) },
    {"getNextRoads", PyWrap( RoadNetwork, getNextRoads, "Get the next roads", vector<VREntityPtr>, VREntityPtr ) },
    {"addRoute", PyWrap( RoadNetwork, addRoute, "Add route path entity from graph node IDs", VREntityPtr, vector<int> ) },
    {"setRoadStyle", PyWrap( RoadNetwork, setRoadStyle, "Set road style flags", void, int ) },
    {"setTerrainOffset", PyWrap( RoadNetwork, setTerrainOffset, "Set road to terrain offset", void, float ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyTrafficSigns::methods[] = {
    {"addSign", PyWrap( TrafficSigns, addSign, "adds trafficSign to world - type - pose", void, string, PosePtr ) },
    {"getName", PyWrap( TrafficSigns, getName, "returns name of shield by ID", string, Vec2i ) },
    {"getVecID", PyWrap( TrafficSigns, getVecID, "returns ID of sign as vector", Vec2i, string ) },
    {"getOSMTag", PyWrap( TrafficSigns, getOSMTag, "returns OSM tag by ID", string, Vec2i ) },
    {"getTextureMosaic", PyWrap( TrafficSigns, getTextureMosaic, "returns textureMosaic", VRTextureMosaicPtr ) },
    {"reloadShader", PyWrap( TrafficSigns, reloadShader, "reloads shader", void ) },
    //{"setMegaTexture", PyWrap( TrafficSigns, setMegaTexture, "sets textureMosaic", void, VRTextureMosaicPtr ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyMapManager::methods[] = {
    {"setServer", PyWrap( MapManager, setServer, "Set server address", void, string ) },
    {"setVault", PyWrap( MapManager, setVault, "Set local path to store map files", void, string ) },
    {"getMap", PyWrap( MapManager, getMap, "Get map file path, retreives file from server if necessary, async if given a callback 'def cb(str):', (N, E, S, callback)", string, double, double, double, VRMessageCbPtr ) },
    {NULL}  /* Sentinel */
};





