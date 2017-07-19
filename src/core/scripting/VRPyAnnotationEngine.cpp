#include "VRPyAnnotationEngine.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"

using namespace OSG;
simpleVRPyType(AnnotationEngine, New_VRObjects_ptr);

PyMethodDef VRPyAnnotationEngine::methods[] = {
    {"set", (PyCFunction)VRPyAnnotationEngine::set, METH_VARARGS, "Set label - set(int i, [x,y,z] pos, str val)" },
    {"clear", (PyCFunction)VRPyAnnotationEngine::clear, METH_NOARGS, "Clear numbers" },
    {"setSize", (PyCFunction)VRPyAnnotationEngine::setSize, METH_VARARGS, "Set font height - setSize( float )" },
    {"setColor", (PyCFunction)VRPyAnnotationEngine::setColor, METH_VARARGS, "Set font color - setColor( [r,g,b,a] )" },
    {"setBackground", (PyCFunction)VRPyAnnotationEngine::setBackground, METH_VARARGS, "Set background color - setBackground( [r,g,b,a] )" },
    {"setBillboard", (PyCFunction)VRPyAnnotationEngine::setBillboard, METH_VARARGS, "Set billboard - setBillboard( bool )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyAnnotationEngine::setBillboard(VRPyAnnotationEngine* self, PyObject* args) {
    self->objPtr->setBillboard(parseBool(args));
    Py_RETURN_TRUE;
}

PyObject* VRPyAnnotationEngine::setSize(VRPyAnnotationEngine* self, PyObject* args) {
    self->objPtr->setSize(parseFloat(args));
    Py_RETURN_TRUE;
}

PyObject* VRPyAnnotationEngine::setColor(VRPyAnnotationEngine* self, PyObject* args) {
    self->objPtr->setColor(Vec4f(parseVec4d(args)));
    Py_RETURN_TRUE;
}

PyObject* VRPyAnnotationEngine::setBackground(VRPyAnnotationEngine* self, PyObject* args) {
    self->objPtr->setBackground(Vec4f(parseVec4d(args)));
    Py_RETURN_TRUE;
}

PyObject* VRPyAnnotationEngine::set(VRPyAnnotationEngine* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyAnnotationEngine::set - Object is invalid"); return NULL; }

    int i;
    const char* s = 0;
    PyObject* p;
    if (! PyArg_ParseTuple(args, "iOs", &i, &p, &s)) return NULL;

    self->objPtr->set(i, parseVec3dList(p), s?s:"");
    Py_RETURN_TRUE;
}

PyObject* VRPyAnnotationEngine::clear(VRPyAnnotationEngine* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyAnnotationEngine::clear - Object is invalid"); return NULL; }
    self->objPtr->clear();
    Py_RETURN_TRUE;
}
