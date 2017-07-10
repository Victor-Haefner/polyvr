#include "VRPyLod.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Lod, New_VRObjects_ptr);

PyMethodDef VRPyLod::methods[] = {
	{"setCenter", PyWrap( Lod, setCenter, "Set the center from which the LOD distance is calculated", void, Vec3f) },
	{"setDistance", PyWrap( Lod, setDistance, "Set the distance at which the specified LOD stage should be shown", void, int, float) },
    {NULL}  /* Sentinel */
};
