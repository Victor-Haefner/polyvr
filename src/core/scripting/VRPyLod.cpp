#include "VRPyLod.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Lod, New_VRObjects_ptr);
simpleVRPyType(LodEvent, 0);
simpleVRPyType(LodTree, New_VRObjects_ptr);
simpleVRPyType(LodLeaf, 0);

PyMethodDef VRPyLod::methods[] = {
	{"setCenter", PyWrap( Lod, setCenter, "Set the center from which the LOD distance is calculated", void, Vec3d) },
	{"setScale", PyWrap( Lod, setScale, "Set a value to scale the distances", void, double) },
	{"setDistance", PyWrap( Lod, setDistance, "Set the distance at which the specified LOD stage should be shown", void, unsigned int, float) },
	{"getCenter", PyWrap( Lod, getCenter, "Get LOD center", Vec3d) },
	{"getScale", PyWrap( Lod, getScale, "Get LOD scale", double) },
	{"getDistance", PyWrap( Lod, getDistance, "Get LOd ith distance", float, unsigned int) },
	{"setCallback", PyWrap( Lod, setCallback, "Set callback, triggers on LOD level switch", void, VRLodCbPtr) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyLodEvent::methods[] = {
	{"getCurrent", PyWrap( LodEvent, getCurrent, "Get event lod level", int) },
	{"getLast", PyWrap( LodEvent, getLast, "Get event last lod level", int) },
	{"getLod", PyWrap( LodEvent, getLod, "Get event lod", VRLodPtr) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyLodTree::methods[] = {
	{"addObject", PyWrapOpt( LodTree, addObject, "Add object, obj, pos, lvl", "1", VRLodLeafPtr, VRTransformPtr, Vec3d, int, bool) },
	{"remObject", PyWrap( LodTree, remObject, "Remove object", VRLodLeafPtr, VRTransformPtr) },
	{"reset", PyWrap( LodTree, reset, "Set the distance at which the specified LOD stage should be shown", void, float) },
	{"getSubTree", PyWrap( LodTree, getSubTree, "Return sub tree", vector<VRLodLeafPtr>, VRLodLeafPtr) },
	{"showOctree", PyWrap( LodTree, showOctree, "Show octree", void) },
	{"size", PyWrap( LodTree, size, "Get number of objects in tree", int) },
	{"rangeSearch", PyWrapOpt( LodTree, rangeSearch, "Get objects in radius around position, pos, radius, depth = -1", "-1", vector<VRObjectPtr>, Vec3d, float, int) },
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
