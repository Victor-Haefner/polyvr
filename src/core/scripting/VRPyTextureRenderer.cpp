#include "VRPyTextureRenderer.h"
#include "VRPyCamera.h"
#include "VRPyMaterial.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRTextureRenderer>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.TextureRenderer",             /*tp_name*/
    sizeof(VRPyTextureRenderer),             /*tp_basicsize*/
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
    "TextureRenderer binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyTextureRenderer::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_VRObjects,                 /* tp_new */
};

PyMethodDef VRPyTextureRenderer::methods[] = {
    {"setup", (PyCFunction)VRPyTextureRenderer::setup, METH_VARARGS, "Set the color of the selection - setup([f,f,f])" },
    {"getMaterial", (PyCFunction)VRPyTextureRenderer::getMaterial, METH_NOARGS, "Select an object - getMaterial( object )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyTextureRenderer::setup(VRPyTextureRenderer* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTextureRenderer::setup - Object is invalid"); return NULL; }
    VRPyCamera* cam; int w, h;
    if (!PyArg_ParseTuple(args, "Oii", &cam, &w, &h)) return NULL;
    self->obj->setup(cam->obj, w, h);
    Py_RETURN_TRUE;
}

PyObject* VRPyTextureRenderer::getMaterial(VRPyTextureRenderer* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTextureRenderer::getMaterial - Object is invalid"); return NULL; }
    return VRPyMaterial::fromPtr( self->obj->getMaterial() );
}
