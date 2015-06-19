#include "VRPyConstructionKit.h"
#include "VRPySnappingEngine.h"
#include "VRPySelector.h"
#include "VRPyGeometry.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRConstructionKit>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.ConstructionKit",             /*tp_name*/
    sizeof(VRPyConstructionKit),             /*tp_basicsize*/
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
    "ConstructionKit binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyConstructionKit::methods,             /* tp_methods */
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

PyMethodDef VRPyConstructionKit::methods[] = {
    {"getSnappingEngine", (PyCFunction)VRPyConstructionKit::getSnappingEngine, METH_NOARGS, "Get internal snapping engine - getSnappingEngine()" },
    {"getSelector", (PyCFunction)VRPyConstructionKit::getSelector, METH_NOARGS, "Get internal selector - getSelector(obj)" },
    {"addAnchorType", (PyCFunction)VRPyConstructionKit::addAnchorType, METH_VARARGS, "Add new anchor type - addAnchorType(size, color)" },
    {"addObjectAnchor", (PyCFunction)VRPyConstructionKit::addObjectAnchor, METH_VARARGS, "Add anchor to object - addObjectAnchor(obj, int anchor, position, flt radius)" },
    {"addObject", (PyCFunction)VRPyConstructionKit::addObject, METH_VARARGS, "Get internal selector - addObject(obj)" },
    {"breakup", (PyCFunction)VRPyConstructionKit::breakup, METH_VARARGS, "Split an object from the system - breakup(obj)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyConstructionKit::getSnappingEngine(VRPyConstructionKit* self) { return VRPySnappingEngine::fromPtr(self->obj->getSnappingEngine()); }
PyObject* VRPyConstructionKit::getSelector(VRPyConstructionKit* self) { return VRPySelector::fromPtr(self->obj->getSelector()); }

PyObject* VRPyConstructionKit::breakup(VRPyConstructionKit* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyConstructionKit::breakup - Object is invalid"); return NULL; }
    OSG::VRGeometry* geo = 0;
    if (!VRPyGeometry::parse(args, &geo)) return NULL;
    self->obj->breakup(geo);
    Py_RETURN_TRUE;
}

PyObject* VRPyConstructionKit::addAnchorType(VRPyConstructionKit* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyConstructionKit::addAnchorType - Object is invalid"); return NULL; }
    float f; PyObject* o = 0;
    if (! PyArg_ParseTuple(args, "fO", &f, &o)) return NULL;
    return PyInt_FromLong( self->obj->addAnchorType(f, parseVec3fList(o)) );
}

PyObject* VRPyConstructionKit::addObjectAnchor(VRPyConstructionKit* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyConstructionKit::addObjectAnchor - Object is invalid"); return NULL; }
    PyObject *o, *p; int a; float d;
    if (! PyArg_ParseTuple(args, "OiOf", &o, &a, &p, &d)) return NULL;
    VRPyGeometry* g = (VRPyGeometry*)o;
    auto anc = self->obj->addObjectAnchor(g->obj, a, parseVec3fList(p), d);
    return VRPyGeometry::fromPtr(anc);
}

PyObject* VRPyConstructionKit::addObject(VRPyConstructionKit* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyConstructionKit::addObject - Object is invalid"); return NULL; }
    OSG::VRGeometry* geo = 0;
    if (!VRPyGeometry::parse(args, &geo)) return NULL;
    self->obj->addObject(geo);
    Py_RETURN_TRUE;
}


