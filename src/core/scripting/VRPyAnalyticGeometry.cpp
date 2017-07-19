#include "VRPyAnalyticGeometry.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(AnalyticGeometry, New_VRObjects_ptr);

PyMethodDef VRPyAnalyticGeometry::methods[] = {
    {"setVector", PyWrap(AnalyticGeometry, setVector, "Add/set an annotated vector", void, int, Vec3d, Vec3d, Vec3d, string) },
    {"setCircle", PyWrap(AnalyticGeometry, setCircle, "Add/set an annotated circle", void, int, Vec3d, Vec3d, float, Vec3d, string) },
    {"setLabelParams", PyWrapOpt(AnalyticGeometry, setLabelParams, "Set the size of the labels", "0|0|0 0 0 1|0 0 0 0", void, float, bool, bool, Vec4d, Vec4d) },
    {"clear", PyWrap(AnalyticGeometry, clear, "Clear data", void) },
    {NULL}  /* Sentinel */
};

// TODO: test default params of setLabelParams

/*PyObject* VRPyAnalyticGeometry::setLabelParams(VRPyAnalyticGeometry* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyAnalyticGeometry::setLabelSize - Object is invalid"); return NULL; }
    float s = 0;
    int ss = false;
    int bb = false;
    PyObject* fgO = 0;
    PyObject* bgO = 0;
    if (! PyArg_ParseTuple(args, "f|iiOO", &s, &ss, &bb, &fgO, &bgO)) return NULL;
    Vec4d fg(0,0,0,1); if (fgO) fg = parseVec4dList(fgO);
    Vec4d bg(0,0,0,0); if (bgO) bg = parseVec4dList(bgO);
    self->objPtr->setLabelParams( s, ss, bb, fg, bg );
    Py_RETURN_TRUE;
}*/
