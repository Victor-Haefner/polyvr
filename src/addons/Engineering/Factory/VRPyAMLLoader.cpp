#include "VRPyAMLLoader.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyTypeCaster.h"

using namespace OSG;

simpleVRPyType(AMLLoader, New_ptr);

PyMethodDef VRPyAMLLoader::methods[] = {
    {"read", PyWrap( AMLLoader, read, "Read AML from file" , void, string ) },
    {"write", PyWrap( AMLLoader, write, "Write AML to file" , void, string ) },
    {NULL}  /* Sentinel */
};
