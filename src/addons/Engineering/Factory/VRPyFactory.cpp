#include "VRPyFactory.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyTypeCaster.h"
#include "core/objects/VRTransform.h"

using namespace OSG;
simpleVRPyType( Factory, New_ptr );

PyMethodDef VRPyFactory::methods[] = {
    {"setupLod", PyWrap( Factory, setupLod, "Setup factory LOD structure", VRObjectPtr, vector<string> ) },
    {NULL}  /* Sentinel */
};

