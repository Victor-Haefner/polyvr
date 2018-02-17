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
    {"setSetting", PyWrap(ProjectManager, setSetting, "Set a setting", void, string, string ) },
    {"getSetting", PyWrap(ProjectManager, getSetting, "Get a setting", string, string ) },
    {"addItem", PyWrapOpt(ProjectManager, addItem, "Add a storable item\n\tmode can be 'RELOAD' or 'REBUILD', reload will only reload the attributes of the object", "RELOAD", void, VRStoragePtr, string ) },
    {"remItem", PyWrap(ProjectManager, remItem, "Remove an item", void, VRStoragePtr ) },
    {"getItems", PyWrap(ProjectManager, getItems, "Get all items", vector<VRStoragePtr> ) },
    {"newProject", PyWrap(ProjectManager, newProject, "New project", void, string ) },
    {"save", PyWrapOpt(ProjectManager, save, "Save to file", "", void, string ) },
    {"load", PyWrapOpt(ProjectManager, load, "Load from file", "", void, string ) },
    {"setPersistencyLevel", PyWrap(ProjectManager, setPersistencyLevel, "Set the persistency level of objects to store, set lower than the persistency of objects to be stored", void, int ) },
    {NULL}  /* Sentinel */
};




