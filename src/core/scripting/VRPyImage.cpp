#include "VRPyImage.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/ndarrayobject.h"

template<> PyTypeObject VRPyBaseT<OSG::Image>::type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "VR.Image",
    sizeof(VRPyImage),
    0,
    (destructor)VRPyImage::dealloc,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
" Constructor:\n\n"
"  VRImage( PyArray data, width, height, pixel format, data type)\n\n"
"  VRImage( PyArray data, width, height, pixel format, data type, internal pixel format)\n\n"
"  pixel formats:\n"
"   A,I,L,LA stands for: GL_ALPHA, GL_INTENSITY, GL_LUMINANCE, GL_LUMINANCE_ALPHA\n"
"   RGB, RGBA, BGR, BGRA\n\n"
"  internal pixel formats:\n"
"   RGB_DXT1, RGBA_DXT1, RGBA_DXT3, RGBA_DXT5\n"
"   DEPTH, DEPTH_STENCIL\n"
"   A_FLT, L_FLT, LA_FLT, RGB_FLT, RGBA_FLT\n"
"   A_INT, L_INT, LA_INT, RGB_INT, RGBA_INT, BGR_INT, BGRA_INT\n\n"
"  data types:\n"
"   UINT8, UINT16, UINT32, FLOAT16, FLOAT32, INT16, INT32, UINT24_8\n",
    0,0,0,0,0,0,
    VRPyImage::methods,
    0,0,0,0,0,0,0,
    (initproc)init,
    0,
    VRPyImage::New,
};

PyMethodDef VRPyImage::methods[] = {
    {NULL}  /* Sentinel */
};

bool CheckExtension(string extN) {
    const char* tmp = (const char*) glGetString(GL_EXTENSIONS);
    if (!tmp) return false;
    string extentions = string(tmp);
    cout << extentions << endl;
    if (string::npos != extentions.find(extN)) return true;
    return false;
}

PyObject* VRPyImage::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    OSG::ImageRecPtr img = OSG::Image::create();

    import_array1(NULL);
    PyArrayObject* data = 0;
    int W, H;
    PyStringObject *channels, *datatype;
    PyStringObject* channels2 = 0;
    if (pySize(args) == 6) { if (! PyArg_ParseTuple(args, "OiiOOO", &data, &W, &H, &channels, &datatype, &channels2)) return NULL; }
    else if (! PyArg_ParseTuple(args, "OiiOO", &data, &W, &H, &channels, &datatype)) return NULL;
    if ((PyObject*)data == Py_None) Py_RETURN_TRUE;
    if ((PyObject*)channels == Py_None) Py_RETURN_TRUE;
    if ((PyObject*)datatype == Py_None) Py_RETURN_TRUE;

    unsigned char* cdata  = (unsigned char*)PyArray_DATA(data);
    int pf = toOSGConst(PyString_AsString((PyObject*)channels));
    int dt = toOSGConst(PyString_AsString((PyObject*)datatype));

    //bool b = CheckExtension(PyString_AsString((PyObject*)channels));
    //cout << "check ext " << PyString_AsString((PyObject*)channels) << " " << b << endl;
    //if (b)
    img->set(pf, W, H, 1, 1, 1, 0, cdata, dt, true);

    VRPyImage* pyimg = (VRPyImage*)alloc( type, img );
    pyimg->img = img;
    if (channels2) pyimg->internal_format = toOSGConst(PyString_AsString((PyObject*)channels2));
    else pyimg->internal_format = -1;
    return (PyObject*)pyimg;
}

void VRPyImage::dealloc(VRPyImage* self) {
    if (self->img != 0) self->img = 0;
    self->ob_type->tp_free((PyObject*)self);
}

