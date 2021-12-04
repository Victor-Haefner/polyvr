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
    {"setupMaterial", PyWrap( PointCloud, setupMaterial, "Setup material, (lit, pointsize)", void, bool, int ) },
    {"applySettings", PyWrap( PointCloud, applySettings, "Setup parameters", void, map<string, string> ) },
    {"convert", PyWrap( PointCloud, convert, "Convert a E57 pointcloud to PCB, it will export to the same path but with '.pcb' at the end", void, string ) },
    {"externalSort", PyWrap( PointCloud, externalSort, "External sort, merge sort, only for PCB files (path, Nchunks, binSize)", void, string, size_t, double ) },
    {"genTestFile", PyWrap( PointCloud, genTestFile, "Generate a pointcloud (.pcb) file (path, Npoints, doColors)", void, string, size_t, bool ) },
    {NULL}  /* Sentinel */
};
