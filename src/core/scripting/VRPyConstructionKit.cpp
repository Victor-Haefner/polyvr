#include "VRPyConstructionKit.h"
#include "VRPySnappingEngine.h"
#include "VRPySelector.h"
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
    {NULL}  /* Sentinel */
};

PyObject* VRPyConstructionKit::getSnappingEngine(VRPyConstructionKit* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyConstructionKit::getSnappingEngine - Object is invalid"); return NULL; }
    return VRPySnappingEngine::fromPtr( self->obj->getSnappingEngine() );
}

PyObject* VRPyConstructionKit::getSelector(VRPyConstructionKit* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyConstructionKit::getSelector - Object is invalid"); return NULL; }
    return VRPySelector::fromPtr( self->obj->getSelector() );
}


