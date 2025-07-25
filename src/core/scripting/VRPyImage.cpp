#include "VRPyImage.h"
#include "VRPyBaseT.h"
#include <OpenSG/OSGImage.h>

#ifndef WITHOUT_NUMPY
#define NO_IMPORT_ARRAY
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/ndarraytypes.h>
#include <numpy/ndarrayobject.h>
#endif

using namespace OSG;

template<> string typeName(const VRTexture* t) { return "Texture"; }
template<> PyObject* VRPyTypeCaster::cast(const VRTexturePtr& e) { return VRPyTexture::fromSharedPtr(e); }

template<> bool toValue(PyObject* o, VRTexturePtr& v) {
    if (VRPyBase::isNone(o)) { v = 0; return 1; }
    if (!VRPyTexture::check(o)) return 0;
    v = ((VRPyTexture*)o)->objPtr;
    return 1;
}

template<> PyTypeObject VRPyBaseT<VRTexture>::type = {
    PyObject_HEAD_INIT(NULL)
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
    {"copy", PyWrap(Texture, copy, "Copy texture", VRTexturePtr ) },
    {"read", PyWrap(Texture, read, "Read an image from disk", bool, string ) },
    {"readGIS", PyWrap(Texture, readGIS, "Read GIS raster data from disk", void, string ) },
    {"readBuffer", PyWrapOpt(Texture, readBuffer, "Read binary file as raster data", "1|0", void, string, string, Vec3i, int, int, int ) },
    {"write", PyWrapOpt(Texture, write, "Write an image to disk - write( str path )", "0", void, string, bool ) },
    {"getPixel", PyWrapOpt(Texture, getPixelVec, "Return pixel at coordinates [i,j,k]", "0", Color4f, Vec3i, bool ) },
    {"getPixelUV", PyWrapOpt(Texture, getPixelUV, "Return pixel at coordinates u,v - getPixel( [u,v] )", "0", Color4f, Vec2d, bool ) },
    {"getPixels", PyWrap(Texture, getPixels, "Return pixels", vector<Color4f>, bool ) },
    {"getSize", PyWrap(Texture, getSize, "Return texture size, [W,H,D]", Vec3i ) },
    {"getAspectRatio", PyWrap(Texture, getAspectRatio, "Return aspect ratio between width and height", float ) },
    {"getChannels", PyWrap(Texture, getChannels, "Get number of image channels", int ) },
    {"mixColor", PyWrap(Texture, mixColor, "Mix texture colors with color", void, Color4f, float ) },
    {"setPixel", PyWrap(Texture, setPixel, "Set pixel, (pos, color)", void, Vec3i, Color4f ) },
    {"setIthPixel", PyWrap(Texture, setIthPixel, "Set ith pixel, (i, color)", void, int, Color4f ) },
    {"setByteData", PyWrapOpt(Texture, setByteData, "Set byte texture data", "1|0", void, vector<char>, Vec3i, int, int, int ) },
    {"setFloatData", PyWrapOpt(Texture, setFloatData, "Set float texture data", "1|0", void, vector<float>, Vec3i, int, int, int ) },
    {"setInternalFormat", PyWrap(Texture, setInternalFormat, "Set internal format", void, int ) },
    {"resize", PyWrapOpt(Texture, resize, "Resize image, (newSize | doScale, offset)", "1|0 0 0", void, Vec3i, bool, Vec3i ) },
    {"turn", PyWrap(Texture, turn, "Turn image n times 90 degree", void, int ) },
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
    //import_array1(NULL);
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
    int pf = toOSGConst(PyUnicode_AsUTF8((PyObject*)channels));
    int dt = toOSGConst(PyUnicode_AsUTF8((PyObject*)datatype));

    //bool b = CheckExtension(PyUnicode_AsUTF8((PyObject*)channels));
    //cout << "check ext " << PyUnicode_AsUTF8((PyObject*)channels) << " " << b << endl;
    //if (b)
    //for (int i=0; i<W*H; i++) cout << "cdata " <<

    img->getImage()->set(pf, W, H, 1, 1, 1, 0, cdata, dt, true);
    if (channels2) img->setInternalFormat( toOSGConst(PyUnicode_AsUTF8((PyObject*)channels2)) );
    return allocPtr( type, img );
#else
    VRTexturePtr img = VRTexture::create();
    return allocPtr( type, img );
#endif
}




