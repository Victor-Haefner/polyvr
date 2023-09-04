#include "VRPyPointCloud.h"
#include "VRPyBaseT.h"

#include "core/math/VRMathFwd.h"

using namespace OSG;

simpleVRPyType(PointCloud, New_VRObjects_ptr);

template<> bool toValue(PyObject* obj, VRPointCloud::Splat& e) {
    toValue<Vec3d>(PyTuple_GetItem(obj, 0), e.p);
    toValue<Color3ub>(PyTuple_GetItem(obj, 1), e.c);
    toValue<Vec2ub>(PyTuple_GetItem(obj, 2), e.v1);
    toValue<Vec2ub>(PyTuple_GetItem(obj, 3), e.v2);
    toValue<char>(PyTuple_GetItem(obj, 4), e.w);
    return true;
}

template<> PyObject* VRPyTypeCaster::cast(const VRPointCloud::Splat& splat) {
    PyObject* pySplat = PyTuple_New(5);
    PyTuple_SetItem(pySplat, 0, VRPyTypeCaster::cast(splat.p));
    PyTuple_SetItem(pySplat, 1, VRPyTypeCaster::cast(splat.c));
    PyTuple_SetItem(pySplat, 2, VRPyTypeCaster::cast(splat.v1));
    PyTuple_SetItem(pySplat, 3, VRPyTypeCaster::cast(splat.v2));
    PyTuple_SetItem(pySplat, 4, VRPyTypeCaster::cast(splat.w));
    return pySplat;
}

PyMethodDef VRPyPointCloud::methods[] = {
    {"getOctreeVisual", PyWrap( PointCloud, getOctreeVisual, "Visual of internal octree", VRGeometryPtr ) },
    {"analyse", PyWrap( PointCloud, analyse, "Analyse a PCB file (path, printOctree)", void, string, bool ) },
    {"addPoint", PyWrap( PointCloud, addPoint, "Add a point, position and color", void, Vec3d, Color3ub ) },
    {"addLevel", PyWrapOpt( PointCloud, addLevel, "Add LOD level, (distance, downsampling, stream = false)", "0", void, float, int, bool ) },
    {"setupLODs", PyWrap( PointCloud, setupLODs, "Setup LODs, this will delete the octree content if not disabled using the settings", void ) },
    {"setupMaterial", PyWrapOpt( PointCloud, setupMaterial, "Setup material, (lit, pointsize, doSplats = false, splatSizeModifier = 0.001)", "0|0.001", void, bool, int, bool, float ) },
    {"getMaterial", PyWrap( PointCloud, getMaterial, "Get material", VRMaterialPtr ) },
    {"applySettings", PyWrap( PointCloud, applySettings, "Setup parameters", void, map<string, string> ) },
    {"convert", PyWrapOpt( PointCloud, convert, "Convert a E57 pointcloud to PCB, if out path provided it will export to the same path but with '.pcb' at the end", "", void, string, string ) },
    {"convertMerge", PyWrapOpt( PointCloud, convertMerge, "Convert E57 pointclouds to a single PCB, if out path provided it will export to the same path but with '.pcb' at the end", "", void, vector<string>, string ) },
    {"externalTransform", PyWrap( PointCloud, externalTransform, "External transform of the points (path, pose)", void, string, PosePtr ) },
    {"externalSort", PyWrap( PointCloud, externalSort, "External sort, merge sort, only for PCB files, chunkSize in bytes to sort internal, bin size in m (path, chunkSize, binSize)", void, string, size_t, double ) },
    {"externalPartition", PyWrapOpt( PointCloud, externalPartition, "External partitioning, only for PCB files", "0", void, string, float) },
    {"externalComputeSplats", PyWrapOpt( PointCloud, externalComputeSplats, "External algorithm to compute surface tangents and splat sizes, (path, splatRadius = 0.1, averageColors = false)", "0.1|0", void, string, float, bool) },
    {"externalColorize", PyWrap( PointCloud, externalColorize, "External algorithm to colorize a pointcloud from panorama images, use the extras/utils/extractEXIF.sh script and give the path to the resulting dat file as second argument", void, string, string, PosePtr, float, float, float, int, int) },
    {"genTestFile", PyWrap( PointCloud, genTestFile, "Generate a pointcloud, regular cube grid, (.pcb) file (path, Npoints, doColors, pDist)", void, string, size_t, bool, float ) },
    {"genTestFile2", PyWrap( PointCloud, genTestFile2, "Generate a pointcloud, ico sphere surface, (.pcb) file (path, Niterations, doColors, splatSize[mm])", void, string, size_t, bool, int ) },
    {"projectOnPanorama", PyWrap( PointCloud, projectOnPanorama, "Project point onto panorama image", Vec3ub, Vec3d, VRTexturePtr, PosePtr ) },
    {"computeSplat", PyWrap( PointCloud, computeSplat, "Compute splat, normal and size, from point and neightbors", VRPointCloud::Splat, Vec3d, vector<VRPointCloud::Splat> ) },
    {"averageColor", PyWrap( PointCloud, averageColor, "Average colors accross list of splats", Color3ub, Vec3d, vector<VRPointCloud::Splat> ) },
    {"radiusSearch", PyWrap( PointCloud, radiusSearch, "Executes a radius search and returns point positions around P inside radius r, (P, r)", vector<VRPointCloud::Splat>, Vec3d, double ) },
    {"externalRadiusSearch", PyWrapOpt( PointCloud, externalRadiusSearch, "Executes a radius search and returns point positions around P inside radius r, (path, P, r)", "0", vector<VRPointCloud::Splat>, string, Vec3d, double, bool ) },
    {"getExternalChunk", PyWrap( PointCloud, getExternalChunk, "Returns point positions of chunk at p, (path, p)", vector<VRPointCloud::Splat>, string, Vec3d) },
    {NULL}  /* Sentinel */
};



