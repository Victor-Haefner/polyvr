#include "VRPyMouse.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Mouse, 0);

PyMethodDef VRPyMouse::methods[] = {
    {"setCursor", (PyCFunction)VRPyMouse::setCursor, METH_VARARGS, "Set the mouse cursor - setCursor(str cursor)\n\tcursor can be: 'WATCH', 'PLUS', ...  \n\tsee here for more: https://developer.gnome.org/gdk3/stable/gdk3-Cursors.html#GdkCursorType" },
    {NULL}  /* Sentinel */
};


PyObject* VRPyMouse::setCursor(VRPyMouse* self, PyObject* args) {
    if (!self->valid()) return NULL;
    const char* c = 0;
    if (! PyArg_ParseTuple(args, "s:setCursor", &c)) return NULL;
    self->objPtr->setCursor(c);
    Py_RETURN_TRUE;
}




