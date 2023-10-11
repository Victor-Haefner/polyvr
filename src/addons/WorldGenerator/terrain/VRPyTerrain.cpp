#include "VRPyTerrain.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(Terrain, New_VRObjects_ptr);
simpleVRPyType(Planet, New_VRObjects_ptr);
simpleVRPyType(Orbit, New_ptr);
simpleVRPyType(MapManager, New_ptr);
simpleVRPyType(MapDescriptor, 0);

PyMethodDef VRPyTerrain::methods[] = {
    {"setParameters", PyWrapOpt(Terrain, setParameters, "Set the terrain parameters, size, resolution, height scale, water level", "1|0|0.0001|0.7 0.9 1|1", void, Vec2d, double, double, float, float, Color3f, bool ) },
    {"setWaterLevel", PyWrap(Terrain, setWaterLevel, "Set the water level", void, float ) },
    {"setLit", PyWrap(Terrain, setLit, "Set lit or not", void, bool ) },
    {"setInvertY", PyWrap(Terrain, setInvertY, "Set invert y tex coord in shader, (sat, topo)", void, bool, bool ) },
    {"setAtmosphericEffect", PyWrap(Terrain, setAtmosphericEffect, "Set the atmospheric density and color", void, float, Color3f ) },
    {"loadMap", PyWrapOpt(Terrain, loadMap, "Load height map opt parameters ( path, channel = 0, consoleOutputEnabled = 1 )", "0|1", void, string, int, bool ) },
    {"setMap", PyWrapOpt(Terrain, setMap, "Set height map, optional channel and rectangle", "0|0 0 1 1", void, VRTexturePtr, int, Vec4d ) },
    {"getMap", PyWrap(Terrain, getMap, "Get height map", VRTexturePtr ) },
    {"setTexture", PyWrapOpt(Terrain, setTexture, "Set color texture", "1 1 1 1|0|0 0 1 1", void, VRTexturePtr, Color4f, float, Vec4d ) },
    {"getTexture", PyWrap(Terrain, getTexture, "Get color texture", VRTexturePtr ) },
    {"physicalize", PyWrap(Terrain, physicalize, "Physicalize terrain", void, bool ) },
    {"projectOSM", PyWrap(Terrain, projectOSM, "Load an OSM file and project surface types onto terrain, OSM path, N, E", void ) },
    {"paintHeights", PyWrap(Terrain, paintHeights, "Simple function to paint by heights using a texture", void, string, string ) },
    {"getSize", PyWrap(Terrain, getSize, "Get terrain size", Vec2d ) },
    {"getHeight", PyWrapOpt(Terrain, getHeight, "Get height at point (local space)", "1", double, Vec2d, bool ) },
    {"getNormal", PyWrap(Terrain, getNormal, "Get normal at point (local space)", Vec3d, Vec3d ) },
    {"getTexCoord", PyWrap(Terrain, getTexCoord, "Get tex coord at point (local space)", Vec2d, Vec2d ) },
    {"probeHeight", PyWrap(Terrain, probeHeight, "Probe height at point, for debugging", vector<Vec3d>, Vec2d ) },
    {"elevatePoint", PyWrapOpt(Terrain, elevatePoint, "Elevate a point", "0|1", Vec3d, Vec3d, float, bool ) },
    {"elevatePose", PyWrapOpt(Terrain, elevatePose, "Elevate a pose", "0", void, PosePtr, float ) },
    {"elevateObject", PyWrapOpt(Terrain, elevateObject, "Elevate an Object onto the terrain", "0", void, VRTransformPtr, float ) },
    {"flatten", PyWrap(Terrain, flatten, "Flatten the area inside a perimeter", void, vector<Vec2d>, float ) },
    {"setHeightScale", PyWrap(Terrain, setHeightScale, "Set height scale", void, float ) },
    {"setupGeo", PyWrapOpt(Terrain, setupGeo, "Computes terrain mesh, pass camera to optimize mesh", "0", void, VRCameraPtr ) },
    {NULL}  /* Sentinel */
};

typedef map<string, VROrbitPtr> mapStrOrbit;

PyMethodDef VRPyPlanet::methods[] = {
    {"addSector", PyWrap(Planet, addSector, "Add sector to planet", VRWorldGeneratorPtr, double, double, bool ) },
    {"addOSMMap", PyWrap(Planet, addOSMMap, "Add OSMMap to planet", OSMMapPtr, string ) },
    {"getRadius", PyWrap(Planet, getRadius, "Return planet radius", double ) },
    {"getSector", PyWrap(Planet, getSector, "Return sector at N E", VRWorldGeneratorPtr, double, double ) },
    {"getSectors", PyWrap(Planet, getSectors, "Return all sectors", vector<VRWorldGeneratorPtr> ) },
    {"getSurfacePose", PyWrapOpt(Planet, getSurfacePose, "Get pose on surface, default in planet coordinates, optionally local, local in sector", "0|0", PosePtr, double, double, bool, bool ) },
    {"getSurfaceUV", PyWrap(Planet, getSurfaceUV, "Get uv of surface terrain texture", Vec2d, double, double ) },
    {"getMaterial", PyWrap(Planet, getMaterial, "Get planet material", VRMaterialPtr ) },
    {"setParameters", PyWrapOpt(Planet, setParameters, "Set planet parameters: radius, texture, isLit, sector size", "0.1", void, double, string, bool, double ) },
    {"setLayermode", PyWrap(Planet, setLayermode, "Set planet layer mode: full, minimum", void, string ) },
    {"setLit", PyWrap(Planet, setLit, "Set planet lit", void, bool ) },
    {"addPin", PyWrapOpt(Planet, addPin, "Add a pin: label, north, east, length", "10000", int, string, double, double, double) },
    {"remPin", PyWrap(Planet, remPin, "Remove a pin: ID", void, int) },
    {"fromLatLongPosition", PyWrapOpt(Planet, fromLatLongPosition, "Get Position on planet based on lat and long, optionally local", "0", Vec3d, double, double, bool) },
    {"fromLatLongNormal", PyWrapOpt(Planet, fromLatLongNormal, "Get Normal on planet based on lat and long, optionally local", "0", Vec3d, double, double, bool) },
    {"fromLatLongSize", PyWrap(Planet, fromLatLongSize, "Get size in meter between p1 and p2 in lat long", Vec2d, double, double, double, double) },
    {"fromPosLatLong", PyWrapOpt(Planet, fromPosLatLong, "Convert space position to lat and long, optionally local", "0|1", Vec2d, Pnt3d, bool, bool) },
    {"localize", PyWrap(Planet, localize, "Center the planet origin on a sector", void, double, double) },
    {"divideTIFF", PyWrap(Planet, divideTIFF, "loads sat images as .tif, dividing into .png chunks - string pathIn, string pathOut, double minLat, double maxLat, double minLon, double maxLon, double resolution \n        pathOut only placeholder right now, new files are saved in project directory", void, string, string, double, double, double, double, double) },
    {"divideTIFFEPSG", PyWrap(Planet, divideTIFFEPSG, "loads images as .tif, dividing into .tif chunks - string pathIn, string pathOut, double minEasting, double maxEasting, double minNorthing, double maxNorthing, double pixelResolution, double chunkResolution", void, string, string, double, double, double, double, double, double, bool) },
    {"setRotation", PyWrap(Planet, setRotation, "Set rotation in days", void, double ) },
    {"getRotation", PyWrap(Planet, getRotation, "Get rotation in days", double ) },
    {"setInclination", PyWrap(Planet, setInclination, "Set inclination in rad", void, double ) },
    {"getInclination", PyWrap(Planet, getInclination, "Get inclination in rad", double ) },
    {"addMoon", PyWrap(Planet, addMoon, "Add moon", void, VRTransformPtr ) },
    {"getMoons", PyWrap(Planet, getMoons, "Get all moons", vector<VRTransformPtr> ) },
    {"addSatellite", PyWrap(Planet, addSatellite, "Add satellite", void, VRTransformPtr ) },
    {"getSatellites", PyWrap(Planet, getSatellites, "Get all satellites", vector<VRTransformPtr> ) },
    {"putInOrbit", PyWrap(Planet, putInOrbit, "See Orbit.fromKepler", VROrbitPtr, VRTransformPtr, vector<double> ) },
    {"getOrbits", PyWrap(Planet, getOrbits, "Get orbits", mapStrOrbit  ) },
    {"getGeoCoord", PyWrap(Planet, getGeoCoord, "Get double precision geo coordinate from an attachment named geoCoords if present, shp file import does add it", Vec3d, VRGeometryPtr, size_t  ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyOrbit::methods[] = {
    {"fromCircle", PyWrap(Orbit, fromCircle, "Parameterize the orbit from a circle, inclination, radius[au], speed[deg]", void, Vec3d, double, double) },
    {"fromKepler", PyWrap(Orbit, fromKepler, "Parameterize the orbit from the 6 Keplerian elements and their rates [/century], "
                                             "semi-major axis[au], eccentricity[deg], inclination[deg], "
                                             "mean longitude[deg], longitude of perihelion[deg], longitude of the ascending node[deg]"
                                             "([a0, e0, I0, l0, w0, o0, da, de, dI, dl, dw, do])", void, vector<double> ) },
    {"computeCoords", PyWrap(Orbit, computeCoords, "Compute the coordinates relative to the ecliptic plane based on the Julian Ephemeris date", Vec3d, double ) },
    {"getPeriod", PyWrap(Orbit, getPeriod, "Estimate the period to do a full orbit", double ) },
    {"setReferential", PyWrap(Orbit, setReferential, "Set referential", void, VRObjectPtr ) },
    {"getReferential", PyWrap(Orbit, getReferential, "Get referential", VRObjectPtr ) },
    {"setTarget", PyWrap(Orbit, setTarget, "Set target", void, VRObjectPtr ) },
    {"getTarget", PyWrap(Orbit, getTarget, "Get target", VRObjectPtr ) },
    {"setTrail", PyWrap(Orbit, setTrail, "Set trail", void, VRObjectPtr ) },
    {"getTrail", PyWrap(Orbit, getTrail, "Get trail", VRObjectPtr ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyMapManager::methods[] = {
    {"setServer", PyWrap( MapManager, setServer, "Set server address", void, string ) },
    {"addMapType", PyWrap( MapManager, addMapType, "Add map type (ID, local path to store map files, script name on server, local filename ending, remote filename format)", void, int, string, string, string, string ) },
    {"getMap", PyWrapOpt( MapManager, getMap, "Get map file path, retreives file from server if necessary, async if given a callback 'def cb(str):', (N, E, S, callback, doStore)", "1", VRMapDescriptorPtr, double, double, double, vector<int>, VRMapCbPtr, bool ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyMapDescriptor::methods[] = {
    {"getMap", PyWrap( MapDescriptor, getMap, "Get ith map texture", VRTexturePtr, int ) },
    {"getMapPath", PyWrap( MapDescriptor, getMapPath, "Get ith map path", string, int ) },
    {"getParameters", PyWrap( MapDescriptor, getParameters, "Get chunk parameters, N, E, S", Vec3d ) },
    {NULL}  /* Sentinel */
};





