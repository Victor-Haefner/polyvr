#include "VRPyTerrain.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyBaseFactory.h"

using namespace OSG;

template<> PyObject* VRPyTypeCaster::cast(const VRTerrainPtr& e) { return VRPyTerrain::fromSharedPtr(e); }

simpleVRPyType(Terrain, New_VRObjects_ptr);
simpleVRPyType(Planet, New_VRObjects_ptr);

PyMethodDef VRPyTerrain::methods[] = {
    {"setParameters", PyWrapOpt(Terrain, setParameters, "Set the terrain parameters, size, resolution, height scale, water level", "1|0", void, Vec2d, double, double, float ) },
    {"setWaterLevel", PyWrap(Terrain, setWaterLevel, "Set the water level", void, float ) },
    {"loadMap", PyWrapOpt(Terrain, loadMap, "Load height map", "3", void, string, int ) },
    {"setMap", PyWrapOpt(Terrain, setMap, "Set height map", "3", void, VRTexturePtr, int ) },
    {"physicalize", PyWrap(Terrain, physicalize, "Physicalize terrain", void, bool ) },
    {"projectOSM", PyWrap(Terrain, projectOSM, "Load an OSM file and project surface types onto terrain, OSM path, N, E", void ) },
    {"paintHeights", PyWrap(Terrain, paintHeights, "Simple function to paint by heights using a texture", void, string ) },
    {"getHeight", PyCastWrap(Terrain, getHeight, "Get height at point", float, Vec2d ) },
    {"probeHeight", PyWrap(Terrain, probeHeight, "Probe height at point, for debugging", vector<Vec3d>, Vec2d ) },
    {"elevatePoint", PyCastWrap(Terrain, elevatePoint, "Elevate a point", void, Vec3d, float ) },
    {"elevatePose", PyWrapOpt(Terrain, elevatePose, "Elevate a pose", "0", void, PosePtr, float ) },
    {"elevateObject", PyWrapOpt(Terrain, elevateObject, "Elevate an Object onto the terrain", "0", void, VRTransformPtr, float ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyPlanet::methods[] = {
    {"addSector", PyWrap(Planet, addSector, "Add sector to planet", VRWorldGeneratorPtr, double, double ) },
    {"getSector", PyWrap(Planet, getSector, "Return sector at N E", VRWorldGeneratorPtr, double, double ) },
    {"getMaterial", PyWrap(Planet, getMaterial, "Get planet material", VRMaterialPtr ) },
    {"setParameters", PyWrapOpt(Planet, setParameters, "Set planet parameters: radius, sector size", "0.1", void, double, double ) },
    {"addPin", PyWrapOpt(Planet, addPin, "Add a pin: label, north, east, length", "10000", int, string, double, double, double) },
    {"remPin", PyWrap(Planet, remPin, "Remove a pin: ID", void, int) },
    {"fromLatLongPosition", PyWrapOpt(Planet, fromLatLongPosition, "Get Position on planet based on lat and long", "0", Vec3d, double, double, bool) },
    {"fromPosLatLong", PyWrapOpt(Planet, fromPosLatLong, "Convert space position to lat and long", "0", Vec2d, Pnt3d, bool) },
    {"localize", PyWrap(Planet, localize, "Center the planet origin on a sector", void, double, double) },
    {NULL}  /* Sentinel */
};
