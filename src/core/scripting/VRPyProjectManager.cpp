#include "VRPyProjectManager.h"
#include "VRPyObject.h"
#include "VRPyTypeCaster.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Storage, 0);
simpleVRPyType(ProjectManager, New_ptr);

PyObject* VRPyStorage::fromSharedPtr(VRStoragePtr sto) {
    auto obj = dynamic_pointer_cast<VRObject>(sto);
    if (obj) return VRPyTypeCaster::cast(obj);
    return VRPyBaseT<VRStorage>::fromSharedPtr(sto);
}

PyMethodDef VRPyStorage::methods[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyProjectManager::methods[] = {
    {"setSetting", PyWrap(ProjectManager, setSetting, "Set a setting", void, string, string ) },
    {"getSetting", PyWrapOpt(ProjectManager, getSetting, "Get a setting", "", string, string, string ) },
    {"addItem", PyWrapOpt(ProjectManager, addItem, "Add a storable item\n\tmode can be 'RELOAD' or 'REBUILD', reload will only reload the attributes of the object", "RELOAD", void, VRStoragePtr, string ) },
    {"remItem", PyWrap(ProjectManager, remItem, "Remove an item", void, VRStoragePtr ) },
    {"getItems", PyWrap(ProjectManager, getItems, "Get all items", vector<VRStoragePtr> ) },
    {"newProject", PyWrap(ProjectManager, newProject, "New project", void, string ) },
    {"save", PyWrapOpt(ProjectManager, save, "Save to file", "", void, string ) },
    {"load", PyWrapOpt(ProjectManager, load, "Load from file", "", void, string ) },
    {"setPersistencyLevel", PyWrap(ProjectManager, setPersistencyLevel, "Set the persistency level of objects to store, set lower than the persistency of objects to be stored", void, int ) },
    {NULL}  /* Sentinel */
};




