#include "VRPyImage.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/ndarrayobject.h"

template<> PyTypeObject VRPyBaseT<OSG::Image>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Image",             /*tp_name*/
    sizeof(VRPyImage),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)VRPyImage::dealloc, /*tp_dealloc*/
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
    "Image binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyImage::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    VRPyImage::New,                 /* tp_new */
};

PyMethodDef VRPyImage::methods[] = {
    {NULL}  /* Sentinel */
};

PyObject* VRPyImage::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    OSG::ImageRecPtr img = OSG::Image::create();

    import_array1(NULL);
    PyArrayObject* data = 0;
    int W, H;
    if (! PyArg_ParseTuple(args, "Oii", &data, &W, &H)) return NULL;
    if ((PyObject*)data == Py_None) Py_RETURN_TRUE;
    unsigned char* cdata  = (unsigned char*)PyArray_DATA(data);
    img->set(OSG::Image::OSG_RGB_PF, W, H, 1, 1, 1, 0, cdata, OSG::Image::OSG_UINT8_IMAGEDATA, true);

    VRPyImage* pyimg = (VRPyImage*)alloc( type, img );
    pyimg->img = img;

    return (PyObject*)pyimg;
}

void VRPyImage::dealloc(VRPyImage* self) {
    if (self->img != 0) self->img = 0;
    self->ob_type->tp_free((PyObject*)self);
}

