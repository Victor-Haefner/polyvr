#include "VRPyLod.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Lod, New_VRObjects_ptr);
simpleVRPyType(LodTree, New_VRObjects_ptr);
simpleVRPyType(LodLeaf, 0);

PyMethodDef VRPyLod::methods[] = {
	{"setCenter", PyWrap( Lod, setCenter, "Set the center from which the LOD distance is calculated", void, Vec3d) },
	{"setDistance", PyWrap( Lod, setDistance, "Set the distance at which the specified LOD stage should be shown", void, uint, float) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyLodTree::methods[] = {
	{"addObject", PyWrapOpt( LodTree, addObject, "Add object, obj, pos, lvl", "1", VRLodLeafPtr, VRTransformPtr, Vec3d, int, bool) },
	{"remObject", PyWrap( LodTree, remObject, "Remove object", VRLodLeafPtr, VRTransformPtr) },
	{"reset", PyWrap( LodTree, reset, "Set the distance at which the specified LOD stage should be shown", void, float) },
	{"getSubTree", PyWrap( LodTree, getSubTree, "Return sub tree", vector<VRLodLeafPtr>, VRLodLeafPtr) },
	{"showOctree", PyWrap( LodTree, showOctree, "Show octree", void) },
	{"size", PyWrap( LodTree, size, "Get number of objects in tree", int) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyLodLeaf::methods[] = {
	{"addLevel", PyWrap( LodLeaf, addLevel, "Add LOD level: distance", void, float) },
	{"add", PyWrap( LodLeaf, add, "Add object: obj, lvl", void, VRObjectPtr, int) },
	{"set", PyWrap( LodLeaf, set, "Set Object: obj, lvl", void, VRObjectPtr, int) },
	{"reset", PyWrap( LodLeaf, reset, "Reset", void) },
	{"getLevel", PyWrap( LodLeaf, getLevel, "Get lvl under parent", int) },
    {NULL}  /* Sentinel */
};
