#include "VRPyPointCloud.h"
#include "VRPyBaseT.h"

#include "core/math/VRMathFwd.h"

using namespace OSG;

simpleVRPyType(PointCloud, New_VRObjects_ptr);

PyMethodDef VRPyPointCloud::methods[] = {
    {"getOctreeVisual", PyWrap( PointCloud, getOctreeVisual, "Visual of internal octree", VRGeometryPtr ) },
    {"addPoint", PyWrap( PointCloud, addPoint, "Add a point, position and color", void, Vec3d, Color3ub ) },
    {"addLevel", PyWrapOpt( PointCloud, addLevel, "Add LOD level, (distance, downsampling, stream = false)", "0", void, float, int, bool ) },
    {"setupLODs", PyWrap( PointCloud, setupLODs, "Setup LODs, this will delete the octree content if not disabled using the settings", void ) },
    {"setupMaterial", PyWrapOpt( PointCloud, setupMaterial, "Setup material, (lit, pointsize, doSplats = false, splatSizeModifier = 0.001)", "0|0.001", void, bool, int, bool, float ) },
    {"getMaterial", PyWrap( PointCloud, getMaterial, "Get material", VRMaterialPtr ) },
    {"applySettings", PyWrap( PointCloud, applySettings, "Setup parameters", void, map<string, string> ) },
    {"convert", PyWrapOpt( PointCloud, convert, "Convert a E57 pointcloud to PCB, if out path provided it will export to the same path but with '.pcb' at the end", "", void, string, string ) },
    {"convertMerge", PyWrapOpt( PointCloud, convertMerge, "Convert E57 pointclouds to a single PCB, if out path provided it will export to the same path but with '.pcb' at the end", "", void, vector<string>, string ) },
    {"externalSort", PyWrap( PointCloud, externalSort, "External sort, merge sort, only for PCB files, chunkSize in bytes to sort internal, bin size in m (path, chunkSize, binSize)", void, string, size_t, double ) },
    {"externalPartition", PyWrap( PointCloud, externalPartition, "External partitioning, only for PCB files", void, string) },
    {"externalComputeSplats", PyWrap( PointCloud, externalComputeSplats, "External algorithm to compute surface tangents and splat sizes", void, string) },
    {"externalColorize", PyWrap( PointCloud, externalColorize, "External algorithm to colorize a pointcloud from panorama images, use the extras/utils/extractEXIF.sh script and give the path to the resulting dat file as second argument", void, string, string, PosePtr, float, float, float, int, int) },
    {"genTestFile", PyWrap( PointCloud, genTestFile, "Generate a pointcloud, regular cube grid, (.pcb) file (path, Npoints, doColors)", void, string, size_t, bool ) },
    {"genTestFile2", PyWrap( PointCloud, genTestFile2, "Generate a pointcloud, ico sphere surface, (.pcb) file (path, Niterations, doColors)", void, string, size_t, bool ) },
    {NULL}  /* Sentinel */
};
