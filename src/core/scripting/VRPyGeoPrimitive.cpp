#include "VRPyGeoPrimitive.h"
#include "VRPyGeometry.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(GeoPrimitive, New_VRObjects_ptr);
simpleVRPyType(Handle, New_VRObjects_ptr);

PyMethodDef VRPyHandle::methods[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyGeoPrimitive::methods[] = {
    {"select", PyWrap( GeoPrimitive, select, "Activate or deactivate the editing handles", void, bool ) },
    {"getHandles", PyWrap( GeoPrimitive, getHandles, "Return the editing handles", vector<VRHandlePtr> ) },
    {"setPrimitive", PyWrap( GeoPrimitive, setPrimitive, "Set geometric primitive", void, string ) },
    {NULL}  /* Sentinel */
};
