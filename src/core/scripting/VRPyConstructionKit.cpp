#include "VRPyConstructionKit.h"
#include "VRPySnappingEngine.h"
#include "VRPySelector.h"
#include "VRPyGeometry.h"
#include "VRPyTypeCaster.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(ConstructionKit, New_ptr);

PyMethodDef VRPyConstructionKit::methods[] = {
    {"clear", PyWrap( ConstructionKit, clear, "Clear everything - clear()", void ) },
    {"getSnappingEngine", PyWrap( ConstructionKit, getSnappingEngine, "Get internal snapping engine - getSnappingEngine()", VRSnappingEnginePtr ) },
    {"getSelector", PyWrap( ConstructionKit, getSelector, "Get internal selector - getSelector(obj)", VRSelectorPtr ) },
    {"getObjects", PyWrap( ConstructionKit, getObjects, "Get all objects - [obj] getObjects()", vector<VRObjectPtr> ) },
    {"addAnchorType", PyWrap( ConstructionKit, addAnchorType, "Add new anchor type - addAnchorType(size, color)", int, float, Color3f ) },
    {"addObjectAnchor", PyWrap( ConstructionKit, addObjectAnchor, "Add anchor to object - addObjectAnchor(obj, int anchor, position, flt radius)", VRGeometryPtr, VRTransformPtr, int, Vec3d, float ) },
    {"addObject", PyWrap( ConstructionKit, addObject, "Get internal selector - addObject(obj)", void, VRTransformPtr ) },
    {"remObject", PyWrap( ConstructionKit, remObject, "Get internal selector - remObject(obj)", void, VRTransformPtr ) },
    {"breakup", PyWrap( ConstructionKit, breakup, "Split an object from the system - breakup(obj)", void, VRTransformPtr ) },
    {"toggleConstruction", PyWrap( ConstructionKit, toggleConstruction, "Toggle construction mode", void, bool ) },
    {NULL}  /* Sentinel */
};


