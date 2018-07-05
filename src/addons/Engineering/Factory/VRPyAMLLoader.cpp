#include "VRPyAMLLoader.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyTypeCaster.h"

using namespace OSG;
simpleVRPyType(AMLLoader, New_toZero);

PyMethodDef VRPyAMLLoader::methods[] = {
    {"load", PyWrap( AMLLoader, load, "Load AML file" , VRObjectPtr, string ) },
    {NULL}  /* Sentinel */
};
