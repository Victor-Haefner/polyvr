#include "VRPyObjectManager.h"
#include "VRPyBaseT.h"
#include "VRPyObject.h"
#include "VRPyTransform.h"
#include "VRPyTypeCaster.h"
#include "VRPyPose.h"

using namespace OSG;

simpleVRPyType(ObjectManager, New_VRObjects_ptr);

PyMethodDef VRPyObjectManager::methods[] = {
    {"add", PyWrap(ObjectManager, add, "Add a copy of the passed object to the managed object pool and return it", VRTransformPtr, VRTransformPtr ) },
    {"copy", PyWrapOpt(ObjectManager, copy, "Add a copy of the passed object to the managed object pool and return it", "1", VRTransformPtr, string, PosePtr, bool ) },
    {"clear", PyWrapOpt(ObjectManager, clear, "Clear objects", "1", void, bool ) },
    {"remove", PyWrap(ObjectManager, rem, "Remove an object", void, VRTransformPtr ) },
    {"addTemplate", PyWrapOpt(ObjectManager, addTemplate, "Add template", "", void, VRTransformPtr, string ) },
    {"getTemplate", PyWrap(ObjectManager, getTemplate, "Return all template objects", VRTransformPtr, string ) },
    {"getCatalog", PyWrap(ObjectManager, getCatalog, "Return all template objects", vector<VRTransformPtr> ) },
    {"updateObject", PyWrap(ObjectManager, updateObject, "Update object data", void, VRTransformPtr ) },
    {NULL}  /* Sentinel */
};



