#include "VRPyCaveKeeper.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simplePyType(CaveKeeper, New_ptr);

PyMethodDef VRPyCaveKeeper::methods[] = {
    {"init", PyWrap2( CaveKeeper, init, "Init real world", void, VRObjectPtr ) },
    {"intersect", PyWrap2( CaveKeeper, intersect, "Intersect a cube", int, VRDevicePtr ) },
    {"remBlock", PyWrap2( CaveKeeper, remBlock, "Remove a cube", void, int ) },
    {"addBlock", PyWrap2( CaveKeeper, addBlock, "Add a cube", int, Vec3i ) },
    {"addObject", PyWrap2( CaveKeeper, place, "Place an object", void, VRDevicePtr, string, VRTransformPtr ) },
    {NULL}  /* Sentinel */
};
