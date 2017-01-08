#include "VRPyFactory.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyTypeCaster.h"
#include "core/objects/VRTransform.h"

using namespace OSG;

template<> PyTypeObject VRPyBaseT<VRFactory>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Factory.Factory",             /*tp_name*/
    sizeof(VRPyFactory),             /*tp_basicsize*/
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
    "Factory binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyFactory::methods,             /* tp_methods */
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

PyMethodDef VRPyFactory::methods[] = {
    {"loadVRML", (PyCFunction)VRPyFactory::loadVRML, METH_VARARGS, "Load VRML file" },
    {"setupLod", (PyCFunction)VRPyFactory::setupLod, METH_VARARGS, "Setup factory LOD structure" },
    {NULL}  /* Sentinel */
};


PyObject* VRPyFactory::loadVRML(VRPyFactory* self, PyObject* args) {
    if (!self->valid()) return NULL;
    auto res = VRTransform::create("factory");
    self->objPtr->loadVRML( parseString(args), 0, res );
    return VRPyTypeCaster::cast( res );
}

PyObject* VRPyFactory::setupLod(VRPyFactory* self, PyObject* args) {
    if (!self->valid()) return NULL;
    vector<PyObject*> vec = parseList(args);
    vector<string> svec;
    for (auto o : vec) svec.push_back( PyString_AsString(o) );
    return VRPyTypeCaster::cast( self->objPtr->setupLod( svec ) );
}

