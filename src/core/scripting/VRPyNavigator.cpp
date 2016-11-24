#include "VRPyNavigator.h"
#include "VRPyNavPreset.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Navigator, 0);

PyMethodDef VRPyNavigator::methods[] = {
    {"getPreset", (PyCFunction)VRPyNavigator::getPreset, METH_VARARGS, "Return the preset by name - getPreset( str preset )" },
    {NULL}  /* Sentinel */
};


PyObject* VRPyNavigator::getPreset(VRPyNavigator* self, PyObject* args) {
    if (!self->valid()) return NULL;
    return VRPyNavPreset::fromPtr(self->obj->getNavigation( parseString(args) ));
}
