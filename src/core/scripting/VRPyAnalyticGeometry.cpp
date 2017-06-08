#include "VRPyAnalyticGeometry.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(AnalyticGeometry, New_VRObjects_ptr);

PyMethodDef VRPyAnalyticGeometry::methods[] = {
    {"setVector", PyWrap(AnalyticGeometry, setVector, void, int, Vec3f, Vec3f, Vec3f, string), "Add/set an annotated vector - setVector(int i, [pos], [vec], [col], str label)" },
    {"setCircle", PyWrap(AnalyticGeometry, setCircle, void, int, Vec3f, Vec3f, float, Vec3f, string), "Add/set an annotated circle - setCircle(int i, [pos], [norm], radius, [col], str label)" },
    {"setLabelParams", PyWrap(AnalyticGeometry, setLabelParams, void, float, bool, bool, Vec4f, Vec4f), "Set the size of the labels - setLabelParams( float s, bool screen_size, bool billboard, fg[r,g,b,a], bg[r,g,b,a] )" },
    {"clear", PyWrap(AnalyticGeometry, clear, void), "Clear data" },
    {NULL}  /* Sentinel */
};

// TODO: implement optional parameters in PyWrap!

/*PyObject* VRPyAnalyticGeometry::setLabelParams(VRPyAnalyticGeometry* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyAnalyticGeometry::setLabelSize - Object is invalid"); return NULL; }
    float s = 0;
    int ss = false;
    int bb = false;
    PyObject* fgO = 0;
    PyObject* bgO = 0;
    if (! PyArg_ParseTuple(args, "f|iiOO", &s, &ss, &bb, &fgO, &bgO)) return NULL;
    Vec4f fg(0,0,0,1); if (fgO) fg = parseVec4fList(fgO);
    Vec4f bg(0,0,0,0); if (bgO) bg = parseVec4fList(bgO);
    self->objPtr->setLabelParams( s, ss, bb, fg, bg );
    Py_RETURN_TRUE;
}*/
