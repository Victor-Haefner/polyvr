#include "VRPyStroke.h"
#include "VRPyPath.h"
#include "VRPyGeometry.h"
#include "VRPyBaseT.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/material/VRMaterial.h"

using namespace OSG;

template<> bool toValue(PyObject* o, VRStroke::CAP& v) { if (!PyString_Check(o)) return 0; toValue(PyString_AsString(o), v); return 1; }

simpleVRPyType(Stroke, New_VRObjects_ptr);

PyMethodDef VRPyStroke::methods[] = {
    {"setPath", PyWrap( Stroke, setPath, "Set a single path", void, pathPtr ) },
    {"addPath", PyWrap( Stroke, addPath, "Add a path", void, pathPtr ) },
    {"setPaths", PyWrap( Stroke, setPaths, "Set a list of paths", void, vector<pathPtr> ) },
    {"getPaths", PyWrap( Stroke, getPaths, "Get the list of paths", vector<pathPtr> ) },
    {"strokeProfile", PyWrapOpt( Stroke, strokeProfile, "Stroke along path using a profile", "1|0|0", void, vector<Vec3d>, bool, bool, bool, VRStroke::CAP, VRStroke::CAP) },
    {"strokeStrew", PyWrap( Stroke, strokeStrew, "Stew objects along path", void, VRGeometryPtr ) },
    {"update", PyWrap( Stroke, update, "Update stroke", void ) },
    {"convertToRope", PyWrap( Stroke, convertToRope, "converts this Stroke  to a rope (softbody)", void ) },
    {"addPolygon", PyWrap( Stroke, addPolygon, "Add a polygon", void, VRPolygonPtr ) },
    {NULL}  /* Sentinel */
};
