#include "VRPyHandGeo.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyBaseT.h"
#include "VRPyLeap.h"

using namespace OSG;

simpleVRPyType(HandGeo, New_VRObjects_ptr);

PyMethodDef VRPyHandGeo::methods[] = {
    {"update", PyWrap(HandGeo, update, "Update hand with new leap frame data", void, VRLeapFramePtr ) },
    {"setLeft", PyWrap(HandGeo, setLeft, "Set to left hand model", void ) },
    {"setRight", PyWrap(HandGeo, setRight, "Set to right hand model", void ) },
    {"connectToLeap", PyWrap(HandGeo, connectToLeap, "Connect to leap device", void, VRLeapPtr ) },
    {NULL}  /* Sentinel */
};
