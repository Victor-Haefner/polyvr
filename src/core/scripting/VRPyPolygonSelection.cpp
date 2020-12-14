#include "VRPyPolygonSelection.h"
#include "VRPyGeometry.h"
#include "VRPyPose.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

template<> PyTypeObject VRPyBaseT<OSG::VRPolygonSelection>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.PolygonSelection",             /*tp_name*/
    sizeof(VRPyPolygonSelection),             /*tp_basicsize*/
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
    VRPyPolygonSelection::methods,             /* tp_methods */
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

PyMethodDef VRPyPolygonSelection::methods[] = {
    {"clear", (PyCFunction)VRPyPolygonSelection::clear, METH_NOARGS, "Clear everything" },
    {"setOrigin", (PyCFunction)VRPyPolygonSelection::setOrigin, METH_VARARGS, "Set origin pose for selection" },
    {"addEdge", (PyCFunction)VRPyPolygonSelection::addEdge, METH_VARARGS, "Add edge to selection" },
    {"close", (PyCFunction)VRPyPolygonSelection::close, METH_VARARGS, "Close selection" },
    {"isClosed", (PyCFunction)VRPyPolygonSelection::isClosed, METH_NOARGS, "Check if selection is closed" },
    {"getShape", (PyCFunction)VRPyPolygonSelection::getShape, METH_NOARGS, "Get selection visual" },
    {"getSelectionFrustum", PyWrap(PolygonSelection, getSelectionFrustum, "Get selection frustum", OSG::Frustum) },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPolygonSelection::setOrigin(VRPyPolygonSelection* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPose* p = 0;
    if (! PyArg_ParseTuple(args, "O:setOrigin", &p)) return NULL;
    self->objPtr->setOrigin( *p->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyPolygonSelection::isClosed(VRPyPolygonSelection* self) {
    if (!self->valid()) return NULL;
    return PyBool_FromLong( self->objPtr->isClosed() );
}

PyObject* VRPyPolygonSelection::getShape(VRPyPolygonSelection* self) {
    if (!self->valid()) return NULL;
    OSG::VRGeometryPtr ptr = self->objPtr->getShape();
    return VRPyGeometry::fromSharedPtr( ptr );
}

PyObject* VRPyPolygonSelection::addEdge(VRPyPolygonSelection* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->addEdge( parseVec3d(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyPolygonSelection::clear(VRPyPolygonSelection* self) {
    if (!self->valid()) return NULL;
    self->objPtr->clear();
    Py_RETURN_TRUE;
}

PyObject* VRPyPolygonSelection::close(VRPyPolygonSelection* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyObject* p = 0;
    if (! PyArg_ParseTuple(args, "O:close", &p)) return NULL;
    self->objPtr->close( p->objPtr );
    Py_RETURN_TRUE;
}




