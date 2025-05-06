#include "VRPyStroke.h"
#include "VRPyPath.h"
#include "VRPyBaseT.h"

using namespace OSG;

template<> bool toValue(PyObject* o, VRStroke::CAP& v) { if (!PyUnicode_Check(o)) return 0; toValue(PyUnicode_AsUTF8(o), v); return 1; }

simpleVRPyType(Stroke, New_VRObjects_ptr);

PyMethodDef VRPyStroke::methods[] = {
    {"clear", PyWrap( Stroke, clear, "Clear paths and polygons", void ) },
    {"setPath", PyWrap( Stroke, setPath, "Set a single path", void, PathPtr ) },
    {"addPath", PyWrap( Stroke, addPath, "Add a path", void, PathPtr ) },
    {"setPaths", PyWrap( Stroke, setPaths, "Set a list of paths", void, vector<PathPtr> ) },
    {"getPaths", PyWrap( Stroke, getPaths, "Get the list of paths", vector<PathPtr> ) },
    {"strokeProfile", PyWrapOpt( Stroke, strokeProfile, "Stroke along path using a profile, (profile, closed, lit, useColors, cap1, cap2, doCaps)", "1|0|0|1", void, vector<Vec3d>, bool, bool, bool, VRStroke::CAP, VRStroke::CAP, bool) },
    {"strokeStrew", PyWrap( Stroke, strokeStrew, "Stew objects along path", void, VRGeometryPtr ) },
    {"update", PyWrap( Stroke, update, "Update stroke", void ) },
    {"convertToRope", PyWrap( Stroke, convertToRope, "converts this Stroke  to a rope (softbody)", void ) },
    {"addPolygon", PyWrap( Stroke, addPolygon, "Add a polygon", void, VRPolygonPtr ) },
    {"makeRectProfile", PyWrap( Stroke, makeRectProfile, "Create open rectangular profile (width, height)", vector<Vec3d>, double, double ) },
    {"makeCircleProfile", PyWrap( Stroke, makeCircleProfile, "Create open circle profile (radius, Npoints)", vector<Vec3d>, double, int ) },
    {NULL}  /* Sentinel */
};
