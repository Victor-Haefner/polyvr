#include "VRPyNumberingEngine.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;
simpleVRPyType(NumberingEngine, New_VRObjects_ptr);

PyMethodDef VRPyNumberingEngine::methods[] = {
    {"set", PyWrapOpt( NumberingEngine, set, "Set number: ID, pos, number, decimal places, groupID", "2|0", void, int, Vec3d, float, int, int ) },
    {"clear", PyWrap( NumberingEngine, clear, "Clear numbers", void ) },
    {NULL}
};
