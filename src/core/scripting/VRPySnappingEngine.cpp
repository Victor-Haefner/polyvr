#include "VRPySnappingEngine.h"
#include "VRPyTransform.h"
#include "VRPyDevice.h"
#include "VRPyPose.h"
#include "VRPyBaseT.h"

using namespace OSG;

template<> bool toValue(PyObject* obj, VRSnappingEngine::PRESET& e) {
    string s = PyString_AsString(obj);
    if (s == "SIMPLE_ALIGNMENT") { e = VRSnappingEngine::SIMPLE_ALIGNMENT; return true; }
    if (s == "SNAP_BACK") { e = VRSnappingEngine::SNAP_BACK; return true; }
    return false;
}

template<> bool toValue(PyObject* obj, VRSnappingEngine::Type& e) {
    string s = PyString_AsString(obj);
    if (s == "NONE") { e = VRSnappingEngine::NONE; return true; }
    if (s == "POINT") { e = VRSnappingEngine::POINT; return true; }
    if (s == "LINE") { e = VRSnappingEngine::LINE; return true; }
    if (s == "PLANE") { e = VRSnappingEngine::PLANE; return true; }
    if (s == "POINT_LOCAL") { e = VRSnappingEngine::POINT_LOCAL; return true; }
    if (s == "LINE_LOCAL") { e = VRSnappingEngine::LINE_LOCAL; return true; }
    if (s == "PLANE_LOCAL") { e = VRSnappingEngine::PLANE_LOCAL; return true; }
    return false;
}

template<> bool toValue(PyObject* o, VRSnappingEngine::VRSnapCbPtr& e) {
    //if (!VRPyEntity::check(o)) return 0; // TODO: add checks!
    Py_IncRef(o);
    e = VRSnappingEngine::VRSnapCb::create( "pyExecCall", boost::bind(VRPyBase::execPyCall<VRSnappingEngine::EventSnap>, o, PyTuple_New(1), _1) );
    return 1;
}

template<> PyObject* VRPyTypeCaster::cast(const VRSnappingEngine::EventSnap& e) {
    auto res = PyTuple_New(6);
    PyTuple_SetItem(res, 0, PyInt_FromLong(e.snap));
    PyTuple_SetItem(res, 1, PyInt_FromLong(e.snapID));
    PyTuple_SetItem(res, 2, VRPyTransform::fromSharedPtr(e.o1));
    PyTuple_SetItem(res, 3, VRPyTransform::fromSharedPtr(e.o2));
    PyTuple_SetItem(res, 4, VRPyPose::fromMatrix(e.m));
    PyTuple_SetItem(res, 5, VRPyDevice::fromSharedPtr(e.dev));
    return res;
}

simpleVRPyType(SnappingEngine, New_ptr)

PyMethodDef VRPySnappingEngine::methods[] = {
    {"addObject", PyWrapOpt(SnappingEngine, addObject, "Add an object to be checked for snapping", "1", void, VRTransformPtr, float ) },
    {"remObject", PyWrap(SnappingEngine, remObject, "Remove an object", void, VRTransformPtr ) },
    {"addTree", PyWrapOpt(SnappingEngine, addTree, "Add all subtree objects to be checked for snapping", "1", void, VRObjectPtr, float ) },
    {"setPreset", PyWrap(SnappingEngine, setPreset, "Initiate the engine with a preset - setPreset(str preset)\n   preset can be: 'snap back', 'simple alignment'", void, VRSnappingEngine::PRESET ) },
//    {"addRule", PyWrapOpt(SnappingEngine, addRule, "Add snapping rule - int addRule(str translation, str orientation, prim_t[x,y,z,x0,y0,z0], prim_o[x,y,z,x0,y0,z0], float dist, float weight, obj local)\n\ttranslation/oriantation: 'NONE', 'POINT', 'LINE', 'PLANE', 'POINT_LOCAL', 'LINE_LOCAL', 'PLANE_LOCAL'\n\texample: addRule('POINT', 'POINT', [0,0,0,0,0,0], [0,1,0,0,0,-1], R, 1, None)", "1|0", void, Type, Type, Line, Line, float, float, VRTransformPtr ) },
    {"addRule", PyWrapOpt(SnappingEngine, addRule, "Add snapping rule", "1|0", int, VRSnappingEngine::Type, VRSnappingEngine::Type, Line, Line, float, float, VRTransformPtr ) },
    {"remRule", PyWrap(SnappingEngine, remRule, "Remove a rule - remRule(int ID)", void, int ) },
    {"addObjectAnchor", PyWrap(SnappingEngine, addObjectAnchor, "Remove a rule - addObjectAnchor(obj transform, obj anchor)", void, VRTransformPtr, VRTransformPtr ) },
    {"clearObjectAnchors", PyWrap(SnappingEngine, clearObjectAnchors, "Remove a rule - clearObjectAnchors(obj transform)", void, VRTransformPtr ) },
    {"remLocalRules", PyWrap(SnappingEngine, remLocalRules, "Remove all object relative rules - clearObjectAnchors(obj transform)", void, VRTransformPtr ) },
    {"addCallback", PyWrap(SnappingEngine, addCallback, "Set snap callback", void, VRSnappingEngine::VRSnapCbPtr ) },
    {NULL}  /* Sentinel */
};

