#include "VRPyProjectManager.h"
#include "VRPyObject.h"
#include "VRPyTypeCaster.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Storage, 0);
simpleVRPyType(StorageContext, 0);
simpleVRPyType(ProjectManager, New_ptr);

PyObject* VRPyStorage::fromSharedPtr(VRStoragePtr sto) {
    auto obj = dynamic_pointer_cast<VRObject>(sto);
    if (obj) return VRPyTypeCaster::cast(obj);
    return VRPyBaseT<VRStorage>::fromSharedPtr(sto);
}

PyMethodDef VRPyStorageContext::methods[] = {
    {NULL}
};

PyMethodDef VRPyStorage::methods[] = {
    {"save", PyWrapOpt(Storage, save, "Save to XML Element, optional set persistency level, default 0", "0", void, XMLElementPtr, int ) },
    {"saveUnder", PyWrapOpt(Storage, saveUnder, "Save to node appended to XML Element, optional set persistency level, default 0, and optional tag name override", "0|", XMLElementPtr, XMLElementPtr, int, string ) },
    {"load", PyWrapOpt(Storage, load, "Load from XML Element", "0", void, XMLElementPtr, VRStorageContextPtr ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyProjectManager::methods[] = {
    {"setSetting", PyWrap(ProjectManager, setSetting, "Set a setting", void, string, string ) },
    {"getSetting", PyWrapOpt(ProjectManager, getSetting, "Get a setting", "", string, string, string ) },
    {"addItem", PyWrapOpt(ProjectManager, addItem, "Add a storable item\n\tmode can be 'RELOAD' or 'REBUILD', reload will only reload the attributes of the object", "RELOAD", void, VRStoragePtr, string ) },
    {"remItem", PyWrap(ProjectManager, remItem, "Remove an item", void, VRStoragePtr ) },
    {"getItems", PyWrap(ProjectManager, getItems, "Get all items", vector<VRStoragePtr> ) },
    {"newProject", PyWrapOpt(ProjectManager, newProject, "New project, (path, autosave = False)", "0", void, string, bool ) },
    {"save", PyWrapOpt(ProjectManager, save, "Save to file", "", void, string ) },
    {"load", PyWrapOpt(ProjectManager, load, "Load from file", "", bool, string ) },
    {"saveTo", PyWrap(ProjectManager, saveTo, "Save to xml node", void, XMLElementPtr ) },
    {"loadFrom", PyWrap(ProjectManager, loadFrom, "Load from xml node", bool, XMLElementPtr ) },
    {"setPersistencyLevel", PyWrap(ProjectManager, setPersistencyLevel, "Set the persistency level of objects to store, set lower than the persistency of objects to be stored", void, int ) },
    {NULL}  /* Sentinel */
};




