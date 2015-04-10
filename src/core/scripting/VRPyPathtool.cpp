#include "VRPyPathtool.h"
#include "VRPyObject.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyPath.h"
#include "VRPyBaseT.h"

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
    {"newPath", (PyCFunction)VRPyPathtool::newPath, METH_VARARGS, "Add a new path - int newPath(device, anchor)" },
    {"remPath", (PyCFunction)VRPyPathtool::remPath, METH_VARARGS, "Remove a path - remPath(int id)" },
    {"extrude", (PyCFunction)VRPyPathtool::extrude, METH_VARARGS, "Extrude a path - extrude(device, int id)" },
    {"addPath", (PyCFunction)VRPyPathtool::addPath, METH_VARARGS, "Extrude a path - extrude(device, int id)" },
    {"select", (PyCFunction)VRPyPathtool::select, METH_VARARGS, "Extrude a path - extrude(device, int id)" },
    {"setVisible", (PyCFunction)VRPyPathtool::setVisible, METH_VARARGS, "Extrude a path - extrude(device, int id)" },
    {"getPaths", (PyCFunction)VRPyPathtool::getPaths, METH_NOARGS, "Extrude a path - extrude(device, int id)" },
    {"getHandles", (PyCFunction)VRPyPathtool::getHandles, METH_VARARGS, "Extrude a path - extrude(device, int id)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPathtool::getPaths(VRPyPathtool* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::getPaths - Object is invalid"); return NULL; }
    vector<OSG::path*> objs = self->obj->getPaths();

    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) {
        PyList_SetItem(li, i, VRPyPath::fromPtr(objs[i]));
    }

    return li;
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
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::select - Object is invalid"); return NULL; }
    self->obj->setVisible( parseBool(args) );
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
    VRPyDevice* dev;
    VRPyObject* obj;
    if (! PyArg_ParseTuple(args, "OO", &dev, &obj)) return NULL;
    OSG::path* p = self->obj->newPath( dev->obj, obj->obj );
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
    self->obj->extrude( dev->obj, p->obj );
    Py_RETURN_TRUE;
}

