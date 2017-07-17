#include "VRPyTerrain.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyBaseFactory.h"

using namespace OSG;

template<> PyObject* VRPyTypeCaster::cast<VRTerrainPtr>(VRTerrainPtr e) { return VRPyTerrain::fromSharedPtr(e); }

simpleVRPyType(Terrain, New_VRObjects_ptr);
simpleVRPyType(Planet, New_VRObjects_ptr);

PyMethodDef VRPyTerrain::methods[] = {
    {"setParameters", PyWrapOpt(Terrain, setParameters, "Set the terrain parameters, size, resolution and height scale", "1", void, Vec2f, float, float) },
    {"loadMap", PyWrapOpt(Terrain, loadMap, "Load height map", "3", void, string, int) },
    {"setMap", PyWrapOpt(Terrain, setMap, "Set height map", "3", void, VRTexturePtr, int) },
    {"physicalize", PyWrap(Terrain, physicalize, "Physicalize terrain", void, bool) },
    {"projectOSM", PyWrap(Terrain, projectOSM, "Load an OSM file and project surface types onto terrain", void, string) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyPlanet::methods[] = {
    {"addSector", PyWrap(Planet, addSector, "Add sector to planet", VRTerrainPtr, int, int) },
    {"getMaterial", PyWrap(Planet, getMaterial, "Get planet material", VRMaterialPtr) },
    {"setParameters", PyWrap(Planet, setParameters, "Set planet parameters: radius", void, float) },
    {"addPin", PyWrap(Planet, addPin, "Add a pin: label, north, east", int, string, float, float) },
    {"remPin", PyWrap(Planet, remPin, "Remove a pin: ID", void, int) },
    {NULL}  /* Sentinel */
};
