#include "VRPyAnalyticGeometry.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"

using namespace OSG;

simpleVRPyType(AnalyticGeometry, New_VRObjects_ptr);

PyMethodDef VRPyAnalyticGeometry::methods[] = {
    {"setVector", (PyCFunction)VRPyAnalyticGeometry::setVector, METH_VARARGS, "Add/set an annotated vector - setVector(int i, [pos], [vec], [col], str label)" },
    {"setCircle", (PyCFunction)VRPyAnalyticGeometry::setCircle, METH_VARARGS, "Add/set an annotated circle - setCircle(int i, [pos], [norm], radius, [col], str label)" },
    {"setLabelParams", (PyCFunction)VRPyAnalyticGeometry::setLabelParams, METH_VARARGS, "Set the size of the labels - setLabelParams( float s, bool screen_size, bool billboard, fg[r,g,b,a], bg[r,g,b,a] )" },
    {"clear", (PyCFunction)VRPyAnalyticGeometry::clear, METH_NOARGS, "Clear data" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyAnalyticGeometry::setLabelParams(VRPyAnalyticGeometry* self, PyObject* args) {
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
}

PyObject* VRPyAnalyticGeometry::setCircle(VRPyAnalyticGeometry* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyAnalyticGeometry::set - Object is invalid"); return NULL; }

    int i; float r;
    PyObject *p, *v, *c, *s;
    if (! PyArg_ParseTuple(args, "iOOfOO", &i, &p, &v, &r, &c, &s)) return NULL;

    self->objPtr->setCircle(i, parseVec3fList(p), parseVec3fList(v), r, parseVec3fList(c), PyString_AsString(s));
    Py_RETURN_TRUE;
}

PyObject* VRPyAnalyticGeometry::setVector(VRPyAnalyticGeometry* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyAnalyticGeometry::set - Object is invalid"); return NULL; }

    int i;
    PyObject *p, *v, *c, *s;
    if (! PyArg_ParseTuple(args, "iOOOO", &i, &p, &v, &c, &s)) return NULL;

    self->objPtr->setVector(i, parseVec3fList(p), parseVec3fList(v), parseVec3fList(c), PyString_AsString(s));
    Py_RETURN_TRUE;
}

PyObject* VRPyAnalyticGeometry::clear(VRPyAnalyticGeometry* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyAnalyticGeometry::clear - Object is invalid"); return NULL; }
    self->objPtr->clear();
    Py_RETURN_TRUE;
}
