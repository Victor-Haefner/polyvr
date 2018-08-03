#include "VRPySpatialCollisionManager.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/math/boundingbox.h"

using namespace OSG;

simpleVRPyType(SpatialCollisionManager, VRPySpatialCollisionManager::New);

PyMethodDef VRPySpatialCollisionManager::methods[] = {
    {"add", PyWrap( SpatialCollisionManager, add, "Add geometry", void, VRObjectPtr ) },
    {"localize", PyWrap( SpatialCollisionManager, localize, "Update collision shape in volume", void, Boundingbox ) },
    {NULL}  /* Sentinel */
};

PyObject* VRPySpatialCollisionManager::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    float f = parseFloat(args);
    return allocPtr( type, VRSpatialCollisionManager::create(f) );
}
