#include "VRPyTextureGenerator.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"
#include "VRPyImage.h"

template<> PyTypeObject VRPyBaseT<OSG::VRTextureGenerator>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.TextureGenerator",             /*tp_name*/
    sizeof(VRPyTextureGenerator),             /*tp_basicsize*/
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
    "TextureGenerator binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyTextureGenerator::methods,             /* tp_methods */
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

PyMethodDef VRPyTextureGenerator::methods[] = {
    {"add", (PyCFunction)VRPyTextureGenerator::add, METH_VARARGS, "Add a layer - add(str type, float amount, [r,g,b], [r,g,b])\n\ttype can be: 'Perlin', 'Bricks'"
                                                                    "\n\t add(str type, float amount, [r,g,b,a], [r,g,b,a])" },
    {"setSize", (PyCFunction)VRPyTextureGenerator::setSize, METH_VARARGS, "Set the size - setSize([width, height, depth])\n   set depth to 1 for 2D textures" },
    {"compose", (PyCFunction)VRPyTextureGenerator::compose, METH_VARARGS, "Bake the layers into an image - img compose( int seed )" },
    {"readSharedMemory", (PyCFunction)VRPyTextureGenerator::readSharedMemory, METH_VARARGS, "Read an image from shared memory - img readSharedMemory( string segment, string data )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyTextureGenerator::compose(VRPyTextureGenerator* self, PyObject* args) {
    return VRPyImage::fromSharedPtr( self->obj->compose( parseInt(args) ) );
}

PyObject* VRPyTextureGenerator::readSharedMemory(VRPyTextureGenerator* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTextureGenerator::add - Object is invalid"); return NULL; }
    const char *segment, *data;
    if (! PyArg_ParseTuple(args, "ss", (char*)&segment, (char*)&data)) return NULL;
    if (segment && data) return VRPyImage::fromSharedPtr( self->obj->readSharedMemory(segment, data) );
    Py_RETURN_NONE;
}

PyObject* VRPyTextureGenerator::add(VRPyTextureGenerator* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTextureGenerator::add - Object is invalid"); return NULL; }
    PyObject *type, *c1, *c2;
    float amount;
    if (! PyArg_ParseTuple(args, "OfOO", &type, &amount, &c1, &c2)) return NULL;
    if (pySize(c1) == 3) self->obj->add(PyString_AsString(type), amount, parseVec3fList(c1), parseVec3fList(c2));
    if (pySize(c1) == 4) self->obj->add(PyString_AsString(type), amount, parseVec4fList(c1), parseVec4fList(c2));
    Py_RETURN_TRUE;
}

PyObject* VRPyTextureGenerator::setSize(VRPyTextureGenerator* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyTextureGenerator::add - Object is invalid"); return NULL; }
    self->obj->setSize( parseVec3i(args) );
    Py_RETURN_TRUE;
}
