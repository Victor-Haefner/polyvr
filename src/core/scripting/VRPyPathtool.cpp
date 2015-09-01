#include "VRPyPathtool.h"
#include "VRPyObject.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyPath.h"
#include "VRPyBaseT.h"
#include "VRPyStroke.h"

template<> PyTypeObject VRPyBaseT<OSG::VRPathtool>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Pathtool",             /*tp_name*/
    sizeof(VRPyPathtool),             /*tp_basicsize*/
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
    "Pathtool binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyPathtool::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New,                 /* tp_new */
};

PyMethodDef VRPyPathtool::methods[] = {
    {"newPath", (PyCFunction)VRPyPathtool::newPath, METH_VARARGS, "Add a new path - path newPath(device, anchor)" },
    {"remPath", (PyCFunction)VRPyPathtool::remPath, METH_VARARGS, "Remove a path - remPath(path)" },
    {"extrude", (PyCFunction)VRPyPathtool::extrude, METH_VARARGS, "Extrude a path - handle extrude(device, path)" },
    {"addPath", (PyCFunction)VRPyPathtool::addPath, METH_VARARGS, "Add a path - addPath(path, object)" },
    {"select", (PyCFunction)VRPyPathtool::select, METH_VARARGS, "Select handle - select(handle)" },
    {"setVisible", (PyCFunction)VRPyPathtool::setVisible, METH_VARARGS, "Set the tool visibility - setVisible(bool)\n     setVisible(bool stroke, bool handles)" },
    {"getPaths", (PyCFunction)VRPyPathtool::getPaths, METH_NOARGS, "Return alist of all paths - [path] getPaths()" },
    {"getHandles", (PyCFunction)VRPyPathtool::getHandles, METH_VARARGS, "Return a list of paths handles - [handle] getHandles(path)" },
    {"getStroke", (PyCFunction)VRPyPathtool::getStroke, METH_VARARGS, "Return the stroke object - stroke getStroke(path)" },
    {"update", (PyCFunction)VRPyPathtool::update, METH_NOARGS, "Update the tool - update()" },
    {"clear", (PyCFunction)VRPyPathtool::clear, METH_VARARGS, "Clear all path nodes - clear(path)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPathtool::clear(VRPyPathtool* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::clear - Object is invalid"); return NULL; }
    VRPyPath* p = 0;
    if (pySize(args) == 1) if (! PyArg_ParseTuple(args, "O", &p)) return NULL;
    OSG::path* pa = 0;
    if (p) pa = p->obj;
    self->obj->clear(pa);
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::update(VRPyPathtool* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::update - Object is invalid"); return NULL; }
    self->obj->update();
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::getPaths(VRPyPathtool* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::getPaths - Object is invalid"); return NULL; }
    vector<OSG::path*> objs = self->obj->getPaths();

    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) {
        PyList_SetItem(li, i, VRPyPath::fromPtr(objs[i]));
    }

    return li;
}

PyObject* VRPyPathtool::getStroke(VRPyPathtool* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::getStroke - Object is invalid"); return NULL; }
    VRPyPath* p = 0;
    if (! PyArg_ParseTuple(args, "O", &p)) return NULL;
    return VRPyStroke::fromPtr( self->obj->getStroke(p->obj) );
}

PyObject* VRPyPathtool::getHandles(VRPyPathtool* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::getHandles - Object is invalid"); return NULL; }

    VRPyPath* p = 0;
    if (pySize(args) == 1) if (! PyArg_ParseTuple(args, "O", &p)) return NULL;
    OSG::path* pa = 0;
    if (p) pa = p->obj;

    vector<OSG::VRGeometry*> objs = self->obj->getHandles(pa);

    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) {
        PyList_SetItem(li, i, VRPyGeometry::fromPtr(objs[i]));
    }

    return li;
}

PyObject* VRPyPathtool::setVisible(VRPyPathtool* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::setVisible - Object is invalid"); return NULL; }
    int b1, b2;
    if (pySize(args) == 2) { if (! PyArg_ParseTuple(args, "ii", &b1, &b2)) return NULL; }
    else {
        if (! PyArg_ParseTuple(args, "i", &b1)) return NULL;
        b2 = b1;
    }
    self->obj->setVisible( b1, b2 );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::select(VRPyPathtool* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::select - Object is invalid"); return NULL; }
    VRPyGeometry* obj;
    if (! PyArg_ParseTuple(args, "O", &obj)) return NULL;
    self->obj->select( obj->obj );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::addPath(VRPyPathtool* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::addPath - Object is invalid"); return NULL; }
    VRPyPath* p;
    VRPyObject* obj;
    if (! PyArg_ParseTuple(args, "OO", &p, &obj)) return NULL;
    self->obj->addPath( p->obj, obj->obj );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::newPath(VRPyPathtool* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::newPath - Object is invalid"); return NULL; }
    VRPyDevice* dev; VRPyObject* obj; int res = 10;
    if (pySize(args) == 2) { if (! PyArg_ParseTuple(args, "OO", &dev, &obj)) return NULL; }
    else if (! PyArg_ParseTuple(args, "OOi", &dev, &obj, &res)) return NULL;
    OSG::VRDevice* d = 0;
    if (!isNone((PyObject*)dev)) d = dev->obj;
    OSG::path* p = self->obj->newPath( d, obj->obj, res );
    return VRPyPath::fromPtr(p);
}

PyObject* VRPyPathtool::remPath(VRPyPathtool* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::remPath - Object is invalid"); return NULL; }
    VRPyPath* p = (VRPyPath*)parseObject(args);
    self->obj->remPath( p->obj );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::extrude(VRPyPathtool* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::extrude - Object is invalid"); return NULL; }
    VRPyDevice* dev; VRPyPath* p;
    if (! PyArg_ParseTuple(args, "OO", &dev, &p)) return NULL;
    OSG::VRDevice* d = 0;
    if (!isNone((PyObject*)dev)) d = dev->obj;
    return VRPyGeometry::fromPtr( self->obj->extrude( d, p->obj) );
}

