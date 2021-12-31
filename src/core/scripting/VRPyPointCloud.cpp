#include "VRPyPointCloud.h"
#include "VRPyBaseT.h"

#include "core/math/VRMathFwd.h"

using namespace OSG;

simpleVRPyType(PointCloud, New_VRObjects_ptr);

PyMethodDef VRPyPointCloud::methods[] = {
    {"getOctree", PyWrap( PointCloud, getOctree, "Access internal octree", OctreePtr ) },
    {"addPoint", PyWrap( PointCloud, addPoint, "Add a point, position and color", void, Vec3d, Color3ub ) },
    {"addLevel", PyWrap( PointCloud, addLevel, "Add LOD level, (distance, downsampling)", void, float, int ) },
    {"setupLODs", PyWrap( PointCloud, setupLODs, "Setup LODs, this will delete the octree content if not disabled using the settings", void ) },
    {"setupMaterial", PyWrapOpt( PointCloud, setupMaterial, "Setup material, (lit, pointsize, doSplats = false, splatSizeModifier = 0.001)", "0|0.001", void, bool, int, bool, float ) },
    {"getMaterial", PyWrap( PointCloud, getMaterial, "Get material", VRMaterialPtr ) },
    {"applySettings", PyWrap( PointCloud, applySettings, "Setup parameters", void, map<string, string> ) },
    {"convert", PyWrapOpt( PointCloud, convert, "Convert a E57 pointcloud to PCB, if out path provided it will export to the same path but with '.pcb' at the end", "", void, string, string ) },
    {"externalSort", PyWrap( PointCloud, externalSort, "External sort, merge sort, only for PCB files, chunkSize in bytes to sort internal, bin size in m (path, chunkSize, binSize)", void, string, size_t, double ) },
    {"genTestFile", PyWrap( PointCloud, genTestFile, "Generate a pointcloud, regular cube grid, (.pcb) file (path, Npoints, doColors)", void, string, size_t, bool ) },
    {"genTestFile2", PyWrap( PointCloud, genTestFile2, "Generate a pointcloud, ico sphere surface, (.pcb) file (path, Niterations, doColors)", void, string, size_t, bool ) },
    {NULL}  /* Sentinel */
};
