#include "VRPySnappingEngine.h"
#include "VRPyTransform.h"
#include "VRPyDevice.h"
#include "VRPyPose.h"
#include "VRPyBaseT.h"

using namespace OSG;

template<> bool toValue(PyObject* obj, VRSnappingEngine::PRESET& e) {
    return toValue( PyUnicode_AsUTF8(obj) , e);
}

template<> bool toValue(PyObject* obj, VRSnappingEngine::Type& e) {
    return toValue( PyUnicode_AsUTF8(obj) , e);
}

template<> bool toValue(PyObject* o, VRSnappingEngine::VRSnapCbPtr& e) {
    //if (!VRPyEntity::check(o)) return 0; // TODO: add checks!
    Py_IncRef(o);
	PyObject* args = PyTuple_New(1);
    e = VRSnappingEngine::VRSnapCb::create( "pyExecCall", bind(VRPyBase::execPyCall<VRSnappingEngine::EventSnapWeakPtr, bool>, o, args, placeholders::_1));
    return 1;
}

template<> PyObject* VRPyTypeCaster::cast(const VRSnappingEngine::EventSnapWeakPtr& we) {
    auto e = we.lock();
    auto res = PyTuple_New(8);
    if (!e) return res;
    PyTuple_SetItem(res, 0, PyLong_FromLong(e->snap));
    PyTuple_SetItem(res, 1, PyLong_FromLong(e->snapID));
    PyTuple_SetItem(res, 2, VRPyTransform::fromSharedPtr(e->o1));
    PyTuple_SetItem(res, 3, VRPyTransform::fromSharedPtr(e->o2));
    PyTuple_SetItem(res, 4, VRPyPose::fromMatrix(e->m));
    PyTuple_SetItem(res, 5, VRPyDevice::fromSharedPtr(e->dev));
    PyTuple_SetItem(res, 6, VRPyTransform::fromSharedPtr(e->a1));
    PyTuple_SetItem(res, 7, VRPyTransform::fromSharedPtr(e->a2));
    return res;
}

simpleVRPyType(SnappingEngine, New_ptr)

PyMethodDef VRPySnappingEngine::methods[] = {
    {"clear", PyWrap(SnappingEngine, clear, "Clear engine", void) },
    {"addObject", PyWrapOpt(SnappingEngine, addObject, "Add an object to be checked for snapping", "0", void, VRTransformPtr, int ) },
    {"remObject", PyWrap(SnappingEngine, remObject, "Remove an object", void, VRTransformPtr ) },
    {"addTree", PyWrapOpt(SnappingEngine, addTree, "Add all subtree objects to be checked for snapping", "0", void, VRObjectPtr, int ) },
    {"setPreset", PyWrap(SnappingEngine, setPreset, "Initiate the engine with a preset - setPreset(str preset)\n   preset can be: 'SNAP_BACK', 'SIMPLE_ALIGNMENT'", void, VRSnappingEngine::PRESET ) },
    {"addRule", PyWrapOpt(SnappingEngine, addRule, "Add snapping rule (Type1, Type2, Pose1, Pose2, snapDistance, group, snapReference), Type can be 'POINT', 'LINE' or 'PLANE'", "0|0", int, VRSnappingEngine::Type, VRSnappingEngine::Type, PosePtr, PosePtr, float, int, VRTransformPtr ) },
    {"remRule", PyWrap(SnappingEngine, remRule, "Remove a rule - remRule(int ID)", void, int ) },
    {"addObjectAnchor", PyWrapOpt(SnappingEngine, addObjectAnchor, "Remove a rule - addObjectAnchor(obj transform, obj anchor)", "0|0", void, VRTransformPtr, VRTransformPtr, int, int ) },
    {"clearObjectAnchors", PyWrap(SnappingEngine, clearObjectAnchors, "Remove a rule (obj transform)", void, VRTransformPtr ) },
    {"pauseObjectAnchors", PyWrap(SnappingEngine, pauseObjectAnchors, "Pause/unpause anchors (obj transform, bool paused)", void, VRTransformPtr, bool ) },
    {"pauseObjectAnchor", PyWrap(SnappingEngine, pauseObjectAnchor, "Pause/unpause ith anchor (obj transform, int i, bool paused)", void, VRTransformPtr, int, bool ) },
    {"remLocalRules", PyWrap(SnappingEngine, remLocalRules, "Remove all object relative rules (obj transform)", void, VRTransformPtr ) },
    {"addCallback", PyWrap(SnappingEngine, addCallback, "Set snap callback", void, VRSnappingEngine::VRSnapCbPtr ) },
    {"enableGhosts", PyWrap(SnappingEngine, enableGhosts, "Set engine to show snapping ghosts", void, bool ) },
    {"setActive", PyWrap(SnappingEngine, setActive, "Set engine active or not", void, bool ) },
    {"isActive", PyWrap(SnappingEngine, isActive, "Returns if engine is active", bool ) },
    {"showSnapping", PyWrap(SnappingEngine, showSnapping, "Show debug visuals of snapping bahavior", void, bool ) },
    {NULL}  /* Sentinel */
};

