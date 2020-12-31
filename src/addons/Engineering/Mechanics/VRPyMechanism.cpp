#include "VRPyMechanism.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyTypeCaster.h"

using namespace OSG;

simpleVRPyType(Mechanism, New_ptr);
simpleVRPyType(GearSegmentation, New_ptr);
simpleVRPyType(AxleSegmentation, New_ptr);

PyMethodDef VRPyMechanism::methods[] = {
    {"add", PyWrapOpt(Mechanism, add, "Add part to mechanism - add(P)", "0", void, VRTransformPtr, VRTransformPtr ) },
    {"update", PyWrap(Mechanism, update, "Update mechanism simulation", void ) },
    {"clear", PyWrap(Mechanism, clear, "Clear mechanism parts", void ) },
    {"addChain", PyWrap(Mechanism, addChain, "Add chain - addChain(float width, [G1, G2, G3, ...])", VRTransformPtr, float, vector<VRTransformPtr>, string ) },
    {"addGear", PyWrapOpt(Mechanism, addGear, "Add custom geo as gear, (geo, width, hole, pitch, N_teeth, teeth_size, bevel, axis, offset)", "0 0 -1|0 0 0", void, VRTransformPtr, float, float, float, int, float, float, Vec3d, Vec3d) },
    {"updateNeighbors", PyWrap(Mechanism, updateNeighbors, "updateNeighbors", void) },
    {"updateVisuals", PyWrap(Mechanism, updateVisuals, "update semantic visuals", void) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyGearSegmentation::methods[] = {
    {"analyse", PyWrap(GearSegmentation, analyse, "Analyse object to get gear parameters", void, VRObjectPtr) },
    {"getAxis", PyWrap(GearSegmentation, getAxis, "Get rotation axis", Vec3d) },
    {"getNGears", PyWrap(GearSegmentation, getNGears, "Get number of gears", int) },
    {"getPlanePositions", PyWrap(GearSegmentation, getPlanePositions, "Get all planes", vector<double>) },
    {"getGearParams", PyWrap(GearSegmentation, getGearParams, "Get ith gear params", vector<double>, int) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyAxleSegmentation::methods[] = {
    {"analyse", PyWrap(AxleSegmentation, analyse, "Analyse object to get axis parameters", void, VRObjectPtr) },
    {NULL}  /* Sentinel */
};

