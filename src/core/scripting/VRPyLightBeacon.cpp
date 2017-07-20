#include "VRPyLightBeacon.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(LightBeacon, New_VRObjects_ptr);

template<> bool toValue(PyObject* o, VRLightBeaconPtr& v) { if (!VRPyLightBeacon::check(o)) return 0; v = ((VRPyLightBeacon*)o)->objPtr; return 1; }

PyMethodDef VRPyLightBeacon::methods[] = {
    {NULL}  /* Sentinel */
};



