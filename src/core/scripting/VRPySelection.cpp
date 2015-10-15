#include "VRPySelection.h"
#include "VRPyGeometry.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

template<> PyTypeObject VRPyBaseT<OSG::VRSelection>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Selection",             /*tp_name*/
    sizeof(VRPySelection),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "ColorChooser binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPySelection::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_ptr,                 /* tp_new */
};

PyMethodDef VRPySelection::methods[] = {
    {"add", (PyCFunction)VRPySelection::add, METH_VARARGS|METH_KEYWORDS, "add to the selection - add(object)" },
    {"sub", (PyCFunction)VRPySelection::sub, METH_VARARGS|METH_KEYWORDS, "substract from the selection - sub(object)" },
    {"clear", (PyCFunction)VRPySelection::clear, METH_NOARGS, "clear selection - clear()" },
    {"getSelected", (PyCFunction)VRPySelection::getSelected, METH_NOARGS, "Return the selected objects - [object] getSelected()" },
    {"getPartialSelected", (PyCFunction)VRPySelection::getPartialSelected, METH_NOARGS, "Return the partially selected - [object] getPartialSelected()" },
    {"getSubselection", (PyCFunction)VRPySelection::getSubselection, METH_VARARGS, "Return the objects selected vertices - [int] getSubselection(object)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPySelection::add(VRPySelection* self, PyObject* args, PyObject* kwargs) {
    if (!self->valid()) return NULL;
    VRPyGeometry* geo = 0;
    PyObject* verts = 0;
    const char* kwlist[] = {"object", "vertices", NULL};
    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:add", (char**)kwlist, &geo, &verts)) return NULL;

    self->objPtr->add(geo->objPtr);
    if (verts) {
        vector<int> vec( PyList_GET_SIZE(verts) );
        for (int i=0; i<PyList_GET_SIZE(verts); i++) vec[i] = PyInt_AsLong( PyList_GetItem(verts,i) );
        self->objPtr->add(geo->objPtr, vec);
    }
    Py_RETURN_TRUE;
}

PyObject* VRPySelection::sub(VRPySelection* self, PyObject* args, PyObject* kwargs) {
    if (!self->valid()) return NULL;
    //VRPyGeometry* geo = 0;
    //if (!PyArg_ParseTuple(args, "O:sub", &geo)) return NULL;
    //self->objPtr->sub(geo->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPySelection::clear(VRPySelection* self) {
    if (!self->valid()) return NULL;
    self->objPtr->clear();
    Py_RETURN_TRUE;
}

PyObject* VRPySelection::getSubselection(VRPySelection* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyGeometry* geo = 0;
    if (!PyArg_ParseTuple(args, "|O:getSubselection", &geo)) return NULL;

    auto toPyArray = [](vector<int>& v) {
        PyObject* res = PyList_New(v.size());
        for (uint i=0; i<v.size(); i++) PyList_SetItem(res, i, PyInt_FromLong(v[i]));
        return res;
    };

    if (geo) {
        auto v = self->objPtr->getSubselection( geo->objPtr );
        return toPyArray( v );
    } else {
        PyObject* res = PyDict_New();
        for (auto m : self->objPtr->getSubselections()) {
            if (m.first) PyDict_SetItem(res, VRPyGeometry::fromSharedPtr(m.first), toPyArray(m.second) );
        }
        return res;
    }
}

PyObject* VRPySelection::getSelected(VRPySelection* self) {
    if (!self->valid()) return NULL;
    auto sel = self->objPtr->getSelected();
    PyObject* res = PyList_New(sel.size());
    for (uint i=0; i<sel.size(); i++) {
        auto sp = sel[i].lock();
        if (sp) PyList_SetItem(res, i, VRPyGeometry::fromSharedPtr(sp));
    }
    return res;
}

PyObject* VRPySelection::getPartialSelected(VRPySelection* self) {
    if (!self->valid()) return NULL;
    auto sel = self->objPtr->getPartials();
    PyObject* res = PyList_New(sel.size());
    for (uint i=0; i<sel.size(); i++) {
        auto sp = sel[i].lock();
        if (sp) PyList_SetItem(res, i, VRPyGeometry::fromSharedPtr(sp));
    }
    return res;
}
