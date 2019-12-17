#include "VRPyPointCloud.h"
#include "VRPyBaseT.h"

#include "core/math/VRMathFwd.h"

using namespace OSG;

simpleVRPyType(PointCloud, New_VRObjects_ptr);

PyMethodDef VRPyPointCloud::methods[] = {
    {"getOctree", PyWrap( PointCloud, getOctree, "Access internal octree", OctreePtr ) },
    {NULL}  /* Sentinel */
};
