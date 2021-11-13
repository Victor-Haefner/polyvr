#include "VRPyPointCloud.h"
#include "VRPyBaseT.h"

#include "core/math/VRMathFwd.h"

using namespace OSG;

simpleVRPyType(PointCloud, New_VRObjects_ptr);

PyMethodDef VRPyPointCloud::methods[] = {
    {"getOctree", PyWrap( PointCloud, getOctree, "Access internal octree", OctreePtr ) },
    {"addPoint", PyWrap( PointCloud, addPoint, "Add a point, position and color", void, Vec3d, Color3f ) },
    {"addLevel", PyWrap( PointCloud, addLevel, "Add LOD level, (distance, downsampling)", void, float, int ) },
    {"setupLODs", PyWrap( PointCloud, setupLODs, "Setup LODs, this will delete the octree content if not disabled using the settings", void ) },
    {"setupMaterial", PyWrap( PointCloud, setupMaterial, "Setup material, (lit, pointsize)", void, bool, int ) },
    {"applySettings", PyWrap( PointCloud, applySettings, "Setup parameters", void, map<string, string> ) },
    {NULL}  /* Sentinel */
};
