#include "VRPyMenu.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Menu, New_VRObjects_ptr);

PyMethodDef VRPyMenu::methods[] = {
    {"append", (PyCFunction)VRPyMenu::append, METH_VARARGS, "Append a child menu - append(str texture_path)" },
    {"setLeafType", (PyCFunction)VRPyMenu::setLeafType, METH_VARARGS, "Set menu layout - setLeafType(str type, vec2f scale)\n\ttype : ['SPRITE'], scale is the size of the sprite" },
    {"setLayout", (PyCFunction)VRPyMenu::setLayout, METH_VARARGS, "Set menu layout - setLayout(str layout, float param)\n\tlayout : ['LINEAR', 'CIRCULAR'], param is the distance between leafs" },
    {"open", (PyCFunction)VRPyMenu::open, METH_NOARGS, "Open menu" },
    {"close", (PyCFunction)VRPyMenu::close, METH_NOARGS, "Close menu" },
    {"setCallback", (PyCFunction)VRPyMenu::setCallback, METH_VARARGS, "Set a menu callback - setCallback(fkt, [params])" },
    {"trigger", (PyCFunction)VRPyMenu::trigger, METH_NOARGS, "Trigger menu or enter next layer if no callback is set" },
    {"move", (PyCFunction)VRPyMenu::setCallback, METH_VARARGS, "Move the cursor - move(int dir)\n\tleft: dir=-1, right: dir=1" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMenu::trigger(VRPyMenu* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMenu::trigger - Object is invalid"); return NULL; }
    self->objPtr->trigger();
    Py_RETURN_TRUE;
}

PyObject* VRPyMenu::move(VRPyMenu* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMenu::move - Object is invalid"); return NULL; }
    self->objPtr->move( parseInt(args) );
    Py_RETURN_TRUE;
}

void execCall(PyObject* pyFkt, PyObject* pArgs, OSG::VRMenuPtr menu) {
    if (pyFkt == 0) return;
    PyGILState_STATE gstate = PyGILState_Ensure();
    if (PyErr_Occurred() != NULL) PyErr_Print();

    if (pArgs == 0) pArgs = PyTuple_New(0);
    PyObject_CallObject(pyFkt, pArgs);

    if (PyErr_Occurred() != NULL) PyErr_Print();
    PyGILState_Release(gstate);
}

PyObject* VRPyMenu::setCallback(VRPyMenu* self, PyObject *args) {
    PyObject *pyFkt, *pArgs = 0;
    if (PyTuple_Size(args) == 1) { if (! PyArg_ParseTuple(args, "O", &pyFkt)) return NULL; }
    else if (! PyArg_ParseTuple(args, "OO", &pyFkt, &pArgs)) return NULL;
    Py_IncRef(pyFkt);

    if (pArgs != 0) {
        std::string type = pArgs->ob_type->tp_name;
        if (type == "list") pArgs = PyList_AsTuple(pArgs);
    }

    Py_IncRef(pArgs);
    self->objPtr->setCallback(new VRFunction<OSG::VRMenuPtr>( "pyMenuCB", bind(execCall, pyFkt, pArgs, placeholders::_1) ));
    Py_RETURN_TRUE;
}

PyObject* VRPyMenu::open(VRPyMenu* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMenu::open - Object is invalid"); return NULL; }
    self->objPtr->open();
    Py_RETURN_TRUE;
}

PyObject* VRPyMenu::close(VRPyMenu* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMenu::close - Object is invalid"); return NULL; }
    self->objPtr->close();
    Py_RETURN_TRUE;
}

PyObject* VRPyMenu::setLeafType(VRPyMenu* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMenu::append - Object is invalid"); return NULL; }
    PyObject *t, *s;
    if (!PyArg_ParseTuple(args, "OO", &t, &s)) return NULL;
    string ts = PyString_AsString(t);
    OSG::VRMenu::TYPE type;
    if (ts == "SPRITE") type = OSG::VRMenu::SPRITE;
    self->objPtr->setLeafType( type, parseVec2dList(s));
    Py_RETURN_TRUE;
}

PyObject* VRPyMenu::setLayout(VRPyMenu* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMenu::append - Object is invalid"); return NULL; }
    PyObject* l; float p;
    if (!PyArg_ParseTuple(args, "Of", &l, &p)) return NULL;
    string ls = PyString_AsString(l);
    OSG::VRMenu::LAYOUT layout;
    if (ls == "LINEAR") layout = OSG::VRMenu::LINEAR;
    if (ls == "CIRCULAR") layout = OSG::VRMenu::CIRCULAR;
    self->objPtr->setLayout( layout, p );
    Py_RETURN_TRUE;
}

PyObject* VRPyMenu::append(VRPyMenu* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMenu::append - Object is invalid"); return NULL; }
    return fromSharedPtr( self->objPtr->append( parseString(args) ) );
}
