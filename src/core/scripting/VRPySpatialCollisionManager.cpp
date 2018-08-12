#include "VRPySpatialCollisionManager.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/math/boundingbox.h"
#include "core/objects/geometry/VRPhysics.h"

using namespace OSG;

simpleVRPyType(SpatialCollisionManager, VRPySpatialCollisionManager::New);

PyMethodDef VRPySpatialCollisionManager::methods[] = {
    {"add", PyWrap( SpatialCollisionManager, add, "Add geometry", void, VRObjectPtr, int ) },
    {"localize", PyWrap( SpatialCollisionManager, localize, "Update collision shape in volume", void, Boundingbox ) },
    {"setCollisionCallback", PyWrap( SpatialCollisionManager, setCollisionCallback, "Set a collision callback", void, VRCollisionCbPtr ) },
    {NULL}  /* Sentinel */
};

PyObject* VRPySpatialCollisionManager::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    float f = parseFloat(args);
    return allocPtr( type, VRSpatialCollisionManager::create(f) );
}
