#include "VRPyNamed.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

using namespace OSG;

simpleVRPyType(Name, 0)

PyMethodDef VRPyName::methods[] = {
    {"getName", PyWrap(Name, getName, "Return name", string ) },
    {"getBaseName", PyWrap(Name, getBaseName, "Return base name", string ) },
    {"getNameSuffix", PyWrap(Name, getNameSuffix, "Return name suffix", int ) },
    {"setName", PyWrap(Name, setName, "Set basename, returns name", string, string ) },
    {NULL}  /* Sentinel */
};
