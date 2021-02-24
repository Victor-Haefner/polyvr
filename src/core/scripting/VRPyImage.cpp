#include "VRPyImage.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"
#include <OpenSG/OSGImage.h>

#ifndef WITHOUT_NUMPY
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/ndarrayobject.h>
#endif

using namespace OSG;

template<> string typeName(const VRTexture& t) { return "Texture"; }
template<> PyObject* VRPyTypeCaster::cast(const VRTexturePtr& e) { return VRPyTexture::fromSharedPtr(e); }
template<> bool toValue(PyObject* o, VRTexturePtr& v) { if (!VRPyTexture::check(o)) return 0; v = ((VRPyTexture*)o)->objPtr; return 1; }

template<> PyTypeObject VRPyBaseT<VRTexture>::type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "VR.Image",
    sizeof(VRPyTexture),
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
    VRPyTexture::methods,
    0,0,0,0,0,0,0,
    (initproc)init,
    0,
    VRPyTexture::New,
};

PyMethodDef VRPyTexture::methods[] = {
    {"read", PyWrap(Texture, read, "Read an image from disk - read( str path )", void, string ) },
    {"write", PyWrapOpt(Texture, write, "Write an image to disk - write( str path )", "0", void, string, bool ) },
    {"getPixel", PyWrap(Texture, getPixelUV, "Return pixel at coordinates u,v - getPixel( [u,v] )", Color4f, Vec2d ) },
    {"getSize", PyWrap(Texture, getSize, "Return texture size, [W,H,D]", Vec3i ) },
    {"getAspectRatio", PyWrap(Texture, getAspectRatio, "Return aspect ratio between width and height", float ) },
    {"getChannels", PyWrap(Texture, getChannels, "Get number of image channels", int ) },
    {"mixColor", PyWrap(Texture, mixColor, "Mix texture colors with color", void, Color4f, float ) },
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

PyObject* VRPyTexture::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
#ifndef WITHOUT_NUMPY
    import_array1(NULL);
    VRTexturePtr img = VRTexture::create();
    if (pySize(args) == 0) return allocPtr( type, img );

    PyArrayObject* data = 0;
    int W, H;
    PyStringObject *channels, *datatype;
    PyStringObject* channels2 = 0;

    if (pySize(args) == 6) { if (! PyArg_ParseTuple(args, "OiiOOO", &data, &W, &H, &channels, &datatype, &channels2)) return NULL; }
    else if (! PyArg_ParseTuple(args, "OiiOO", &data, &W, &H, &channels, &datatype)) return NULL;
    if ((PyObject*)data == Py_None) Py_RETURN_NONE;
    if ((PyObject*)channels == Py_None) Py_RETURN_NONE;
    if ((PyObject*)datatype == Py_None) Py_RETURN_NONE;

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
#else
    VRTexturePtr img = VRTexture::create();
    return allocPtr( type, img );
#endif
}




