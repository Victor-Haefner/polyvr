#include "VRPyProjectManager.h"
#include "VRPyObject.h"
#include "VRPyTypeCaster.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Storage, 0);
simpleVRPyType(ProjectManager, New_ptr);

PyMethodDef VRPyStorage::methods[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyProjectManager::methods[] = {
    {"addItem", PyWrapOpt(ProjectManager, addItem, "Add a storable item\n\tmode can be 'RELOAD' or 'REBUILD', reload will only reload the attributes of the object", "RELOAD", void, VRStoragePtr, string ) },
    {"remItem", PyWrap(ProjectManager, remItem, "Remove an item", void, VRStoragePtr ) },
    {"getItems", PyWrap(ProjectManager, getItems, "Get all items", vector<VRStoragePtr> ) },
    {"newProject", PyWrap(ProjectManager, newProject, "New project", void, string ) },
    {"save", PyWrapOpt(ProjectManager, save, "Save to file", "", void, string ) },
    {"load", PyWrapOpt(ProjectManager, load, "Load from file", "", void, string ) },
    {"setPersistencyLevel", PyWrap(ProjectManager, setPersistencyLevel, "Set the persistency level of objects to store", void, int ) },
    {NULL}  /* Sentinel */
};




