#include "VRPyImage.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"
#include <OpenSG/OSGImage.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/ndarrayobject.h"

template<> PyTypeObject VRPyBaseT<OSG::VRTexture>::type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "VR.Image",
    sizeof(VRPyImage),
    0,
    (destructor)dealloc,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
" Constructor:\n\n"
"  VRImage( )\n\n"
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
"   UINT8, UINT16, UINT32, FLOAT16, FLOAT32, INT16, INT32, UINT24_8\n"
"  example:\n"
"\tdata = np.array([[0,0,0], [1,0,0], [0,0,1], [0,1,0]], np.float32)\n"
"\timg = VR.Image(data, 2,2,'RGB','FLOAT32')\n"
"\tmat.setTexture(img)\n",
    0,0,0,0,0,0,
    VRPyImage::methods,
    0,0,0,0,0,0,0,
    (initproc)init,
    0,
    VRPyImage::New,
};

PyMethodDef VRPyImage::methods[] = {
    {"read", (PyCFunction)VRPyImage::read, METH_VARARGS, "Read an image from disk - read( str path )" },
    {"write", (PyCFunction)VRPyImage::write, METH_VARARGS, "Write an image to disk - write( str path )" },
    {"getPixel", (PyCFunction)VRPyImage::getPixel, METH_VARARGS, "Return pixel at coordinates u,v - getPixel( [u,v] )" },
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
    import_array1(NULL);
    OSG::VRTexturePtr img = OSG::VRTexture::create();
    if (pySize(args) == 0) return allocPtr( type, img );

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
    //for (int i=0; i<W*H; i++) cout << "cdata " <<

    img->getImage()->set(pf, W, H, 1, 1, 1, 0, cdata, dt, true);
    if (channels2) img->setInternalFormat( toOSGConst(PyString_AsString((PyObject*)channels2)) );
    return allocPtr( type, img );
}

PyObject* VRPyImage::read(VRPyImage* self, PyObject *args) {
    const char* path = 0;
    if (! PyArg_ParseTuple(args, "s", (char*)&path)) return NULL;
    if (path) self->objPtr->read(path);
    Py_RETURN_TRUE;
}

PyObject* VRPyImage::write(VRPyImage* self, PyObject *args) {
    const char* path = 0;
    if (! PyArg_ParseTuple(args, "s", (char*)&path)) return NULL;
    if (path) self->objPtr->write(path);
    Py_RETURN_TRUE;
}

PyObject* VRPyImage::getPixel(VRPyImage* self, PyObject *args) {
    PyObject* uv;
    if (! PyArg_ParseTuple(args, "O", &uv)) return NULL;
    return toPyTuple( self->objPtr->getPixel( parseVec2fList(uv) ) );
}




