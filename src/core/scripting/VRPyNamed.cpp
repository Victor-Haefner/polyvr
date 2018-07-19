#include "VRPyNamed.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

using namespace OSG;

simpleVRPyType(Name, 0)

PyMethodDef VRPyName::methods[] = {
    {"getName", PyWrap(Name, getName, "Return device name.", string ) },
    {NULL}  /* Sentinel */
};
