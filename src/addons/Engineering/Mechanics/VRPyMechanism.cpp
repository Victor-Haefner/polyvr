#include "VRPyMechanism.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyTypeCaster.h"

using namespace OSG;

simpleVRPyType(Mechanism, New_ptr);
#ifndef WITHOUT_EIGEN
simpleVRPyType(GearSegmentation, New_ptr);
simpleVRPyType(AxleSegmentation, New_ptr);
#endif

PyMethodDef VRPyMechanism::methods[] = {
    {"add", PyWrapOpt(Mechanism, add, "Add part to mechanism - add(P)", "0", void, VRTransformPtr, VRTransformPtr ) },
    {"update", PyWrap(Mechanism, update, "Update mechanism simulation", void ) },
    {"clear", PyWrap(Mechanism, clear, "Clear mechanism parts", void ) },
    {"addChain", PyWrap(Mechanism, addChain, "Add chain - addChain(float width, [G1, G2, G3, ...])", VRTransformPtr, float, vector<VRTransformPtr>, string ) },
    {"addGear", PyWrapOpt(Mechanism, addGear, "Add custom geo as gear, (geo, width, hole, pitch, N_teeth, teeth_size, bevel, axis, offset)", "0 0 -1|0 0 0", void, VRTransformPtr, float, float, float, int, float, float, Vec3d, Vec3d) },
    {"updateNeighbors", PyWrap(Mechanism, updateNeighbors, "updateNeighbors", void) },
    {"updateVisuals", PyWrap(Mechanism, updateVisuals, "update semantic visuals", void) },
    {"getNParts", PyWrap(Mechanism, getNParts, "get number of parts", int) },
    {NULL}  /* Sentinel */
};

#ifndef WITHOUT_EIGEN
PyMethodDef VRPyGearSegmentation::methods[] = {
    {"analyse", PyWrap(GearSegmentation, analyse, "Analyse object to get gear parameters", void, VRObjectPtr) },
    {"setBinSizes", PyWrap(GearSegmentation, setBinSizes, "Set comparison eps for plane, plane match and radius", void, double, double, double) },
    {"setFFTFreqHint", PyWrap(GearSegmentation, setFFTFreqHint, "Set FFT hints, N freqs to get and ith to choose, (N freqs = 1, ith freq = 0)", void, int, int) },
    {"getAxis", PyWrap(GearSegmentation, getAxis, "Get rotation axis", Vec3d) },
    {"getPolarCoords", PyWrap(GearSegmentation, getPolarCoords, "Get polar coordinate system", PosePtr) },
    {"getNGears", PyWrap(GearSegmentation, getNGears, "Get number of gears", size_t) },
    {"getNPlanes", PyWrap(GearSegmentation, getNPlanes, "Get number of planes", size_t) },
    {"getGearParams", PyWrap(GearSegmentation, getGearParams, "Get ith gear params", vector<double>, size_t) },
    {"getPlanePosition", PyWrap(GearSegmentation, getPlanePosition, "Get plane position along axis", double, size_t) },
    {"getPlaneVertices", PyWrap(GearSegmentation, getPlaneVertices, "Get plane polar vertices", vector<Vec2d>, size_t) },
    {"getPlaneFrequencies", PyWrap(GearSegmentation, getPlaneFrequencies, "Get plane frequencies from sine approx", vector<double>, size_t) },
    {"getPlaneContour", PyWrap(GearSegmentation, getPlaneContour, "Get plane contour", vector<Vec2d>, size_t) },
    {"getPlaneSineGuess", PyWrapOpt(GearSegmentation, getPlaneSineGuess, "Get plane sine guess params", "0", vector<double>, size_t, size_t) },
    {"getPlaneSineApprox", PyWrapOpt(GearSegmentation, getPlaneSineApprox, "Get plane sine fit params", "0", vector<double>, size_t, size_t) },
    {"runTest", PyWrap(GearSegmentation, runTest, "Run test, output in console", void) },
    {"printResults", PyWrap(GearSegmentation, printResults, "Output results to console", void) },
    {"createGear", PyWrap(GearSegmentation, createGear, "Create a gear geometry based on analysis", VRGeometryPtr, size_t) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyAxleSegmentation::methods[] = {
    {"analyse", PyWrap(AxleSegmentation, analyse, "Analyse object to get axis parameters", void, VRObjectPtr) },
    {NULL}  /* Sentinel */
};
#endif

