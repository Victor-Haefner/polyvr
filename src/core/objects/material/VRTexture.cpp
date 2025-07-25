#include "VRTexture.h"
#include "core/math/partitioning/boundingbox.h"
#include "core/utils/toString.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRScene.h"
#ifndef WITHOUT_GDAL
#include "core/scene/import/GIS/VRGDAL.h"
#endif

#include <OpenSG/OSGImage.h>

using namespace OSG;

VRTexture::VRTexture() { img = Image::create(); }
VRTexture::VRTexture(ImageMTRecPtr img) { this->img = img; }
VRTexture::~VRTexture() {}

VRTexturePtr VRTexture::create() { return shared_ptr<VRTexture>(new VRTexture() ); }
VRTexturePtr VRTexture::create(ImageMTRecPtr img) { return shared_ptr<VRTexture>(new VRTexture(img) ); }
VRTexturePtr VRTexture::ptr() { return shared_from_this(); }

VRTexturePtr VRTexture::copy() {
    auto t = create();
    t->img = dynamic_pointer_cast<Image>(img->shallowCopy());
    t->internal_format = internal_format;
    return t;
}

void VRTexture::setImage(ImageMTRecPtr img) { this->img = img; }
void VRTexture::setInternalFormat(int ipf) { internal_format = ipf; }
int VRTexture::getInternalFormat() { return internal_format; }
ImageMTRecPtr VRTexture::getImage() { return img; }

void VRTexture::readBuffer(string path, string format, Vec3i layout, int chanels, int Nmipmaps, int ipf) {
    int frmt = -1;
    if (format == "UINT8") frmt = Image::OSG_UINT8_IMAGEDATA;
    if (format == "FLOAT32") frmt = Image::OSG_FLOAT32_IMAGEDATA;
    if (frmt < 0) { cout << "ERROR: format conversion failed!" << endl; return; }

    ifstream file(path, std::ios::binary);
    if (!file) { cout << "Error opening " << path << endl; return; }

    file.seekg(0, std::ios::end);
    size_t N = file.tellg();

    file.seekg(0, std::ios::beg);
    vector<char> data(N);
    file.read(data.data(), N);
    if (file.fail()) { cout << "Error reading " << path << endl; return;}

    for (int i=0; i<200; i++) cout << " " << (int)(uint8_t)data[i];
    cout << endl;

    file.close();

    int chEnum = 0;
    if (chanels == 1) chEnum = Image::OSG_I_PF;
    if (chanels == 2) chEnum = Image::OSG_LA_PF;
    if (chanels == 3) chEnum = Image::OSG_RGB_PF;
    if (chanels == 4) chEnum = Image::OSG_RGBA_PF;

    img->set( chEnum, layout[0], layout[1], layout[2], Nmipmaps, 1, 0, (const uint8_t*)&data[0], frmt, true, 1);
    internal_format = ipf;
}

void VRTexture::setByteData(vector<char> data, Vec3i layout, int chanels, int Nmipmaps, int ipf) {
    img->set( chanels, layout[0], layout[1], layout[2], Nmipmaps, 1, 0, (const uint8_t*)&data[0], Image::OSG_UINT8_IMAGEDATA, true, 1);
    internal_format = ipf;
}

void VRTexture::setFloatData(vector<float> data, Vec3i layout, int chanels, int Nmipmaps, int ipf) {
    img->set( chanels, layout[0], layout[1], layout[2], Nmipmaps, 1, 0, (const uint8_t*)&data[0], Image::OSG_FLOAT32_IMAGEDATA, true, 1);
    internal_format = ipf;
}

bool VRTexture::read(string path) {
    bool b = img->read(path.c_str());
    if (!b) cout << " VRTexture::read from file '" << path << "' failed!" << endl;
    return b;
}

void VRTexture::readGIS(string path) {
#ifndef WITHOUT_GDAL
    auto tex = loadGeoRasterData(path, false);
    img = tex->img;
    internal_format = tex->internal_format;
#endif
}

void VRTexture::writeThreaded(string path, VRTexturePtr self, VRThreadWeakPtr tw) { // TODO: add mutex!
    string folder = getFolderName(path);
    if (!exists(folder)) makedir(folder);
    writeImage(img, path);
    self.reset();
    writeWorker.reset();
}

void VRTexture::write(string path, bool doThread) {
    if (!doThread) {
        string folder = getFolderName(path);
        if (!exists(folder)) makedir(folder);
        writeImage(img, path);
    } else {
        auto scene = VRScene::getCurrent();
        if (!writeWorker) writeWorker = VRThreadCb::create("writeThreaded", bind(&VRTexture::writeThreaded, this, path, ptr(), placeholders::_1));
        scene->initThread(writeWorker, "write image to disk");
    }
}

bool convertToUInt8(ImageMTRecPtr img) {
    auto dtype = img->getDataType();
    auto format = img->getPixelFormat();
    Vec3i s = Vec3i(img->getWidth(), img->getHeight(), img->getDepth());

    if (dtype == OSG::Image::OSG_FLOAT32_IMAGEDATA && format == OSG::Image::OSG_RGB_PF) {
        size_t N = s[0]*s[1]*s[2];
        Color3f* src = (Color3f*)img->getData();
        vector<Color3ub> dst(N);
        for (size_t i=0; i<N; i++) {
            Color3f c = src[i];
            UInt8 r = max(0.f,c[0])*255;
            UInt8 g = max(0.f,c[1])*255;
            UInt8 b = max(0.f,c[2])*255;
            dst[i] = Color3ub(r,g,b);
        }
        img->set(format, s[0], s[1], s[2], 1, 1, 0, (UInt8*)&dst[0]);
        return true;
    }
    return false;
}

void VRTexture::convertToBytes() {
    if (!convertToUInt8(img)) img->convertDataTypeTo(OSG::Image::OSG_UINT8_IMAGEDATA);
}

void VRTexture::writeImage(ImageMTRecPtr img, string path) {
    //auto format = hasAlpha ? OSG::Image::OSG_RGBA_PF : OSG::Image::OSG_RGB_PF;

    //OSG::Image::OSG_FLOAT32_IMAGEDATA
    auto dtype = img->getDataType();

    if (dtype != OSG::Image::OSG_UINT8_IMAGEDATA) { // need to convert image data to write to file
        cout << "convert image data from " << dtype << " to OSG::Image::OSG_UINT8_IMAGEDATA" << endl;
        ImageMTRecPtr img2 = Image::create();
        img2->set(img);
        if (!convertToUInt8(img2)) img2->convertDataTypeTo(OSG::Image::OSG_UINT8_IMAGEDATA);
        img = img2;
    }

    auto format = img->getPixelFormat();
    if (format == OSG::Image::OSG_A_PF) { // need to convert pixel data
        cout << "convert pixel data from " << format << " to OSG::Image::OSG_RGB_PF" << endl;
        ImageMTRecPtr img2 = Image::create();
        img->reformat(OSG::Image::OSG_RGB_PF, img2);
        img = img2;
    }

    if (!img->write(path.c_str())) cout << " VRTexture::write to file '" << path << "' failed!" << endl;
}

struct Pixel {
    char r;
    char g;
    char b;
    char a;

    Pixel(char r, char g, char b, char a) : r(r), g(g), b(b), a(a) {}
};

void VRTexture::paste(VRTexturePtr other, Vec3i offset) {
    Vec3i s1 = getSize();
    Vec3i s2 = other->getSize();
    if (s2[0]*s2[1]*s2[2] == 0) return;

    Vec3i S = offset + s2;
    S[0] = min(S[0], s1[0]);
    S[1] = min(S[1], s1[1]);
    S[2] = min(S[2], s1[2]);
    Vec3i s3 = S - offset;

    auto data1 = img->editData();
    auto data2 = other->img->editData();
    int Bpp1 = getPixelByteSize();
    int Bpp2 = other->getPixelByteSize();
    int BppMin = min(Bpp1, Bpp2);
    for (int k=0; k<s3[2]; k++) {
        for (int j=0; j<s3[1]; j++) {
            size_t J1 = (offset[0] + (j+offset[1])*s1[0] + (k+offset[2])*s1[0]*s1[1])*Bpp1;
            size_t J2 = (j*s2[0] + k*s2[0]*s2[1])*Bpp2;
            if (Bpp1 == Bpp2) memcpy(data1+J1, data2+J2, s3[0]*Bpp2); // copy whole line
            else {
                for (int i=0; i<s3[0]; i++) {
                    size_t J11 = J1 + i*Bpp1;
                    size_t J21 = J2 + i*Bpp2;

                    if (Bpp1 == 4 && Bpp2 == 1) { // hack, make it nicer later on
                        char c = *(data2+J21);
                        auto p = new Pixel(c,c,c,255);
                        memcpy(data1+J11, p, Bpp1);
                        delete p;
                    }

                    if (Bpp1 == 4 && Bpp2 == 3) { // hack, make it nicer later on
                        auto p = new Pixel(255,255,255,255);
                        memcpy(data1+J11, p, Bpp1);
                        delete p;
                    }

                    memcpy(data1+J11, data2+J21, BppMin); // copy single pixel
                }
            }
        }
    }

    //const UInt8* data = other->getImage()->getData();
    //img->setSubData(offset[0], offset[1], offset[2], dim[0], dim[1], dim[2], data);
    // TODO: evaluate the speed against the setSubData function!
}

void VRTexture::resize(Vec3i size, bool scale, Vec3i offset) {
    cout << "resize " << getSize() << " to " << size << ", scale " << scale << ", offset " << offset << endl;
    ImageMTRecPtr nimg = Image::create();
    nimg->set(img->getPixelFormat(), size[0], size[1], size[2], img->getMipMapCount(), img->getFrameCount(), img->getFrameDelay(), 0, img->getDataType(), true, img->getSideCount());
    VRTexturePtr tmp;

    if (scale) {
        ImageMTRecPtr simg = Image::create();
        simg->set(img->getPixelFormat(), size[0], size[1], size[2], img->getMipMapCount(), img->getFrameCount(), img->getFrameDelay(), 0, img->getDataType(), true, img->getSideCount());
        img->scale(size[0], size[1], size[2], simg);
        tmp = VRTexture::create(simg);
    } else {
        tmp = VRTexture::create(img);
    }

    img = nimg;
    paste(tmp, offset);
}

void VRTexture::merge(VRTexturePtr other, Vec3d pos) {
    if (!other) return;
    if (!other->img) return;
    if (!img || getSize()[0] == 0) { img = other->img; return; }

    Vec3i dim1 = getSize();
    Vec3i dim2 = other->getSize();
    Vec3i offset = Vec3i(dim1[0]*pos[0], dim1[1]*pos[1], dim1[2]*pos[2]);
    Vec3i offset2 = Vec3i(min(0,offset[0]), min(0,offset[1]), min(0,offset[2]));

    Boundingbox bb; // compute total new size
    bb.updateFromPoints( { Vec3d(offset), Vec3d(offset2), Vec3d(offset+dim2), Vec3d(offset2+dim1) } );
    Vec3i dim3 = Vec3i(bb.size());
    resize(dim3, false, offset2);
    paste(other, offset);
}

int VRTexture::getChannels() {
    if (!img) return 0;
    auto f = img->getPixelFormat();
#ifdef __EMSCRIPTEN__
    if (f == Image::OSG_R_PF) return 1;
#else
    if (f == GL_RED) return 1;
#endif
    if (f == Image::OSG_A_PF) return 1;
    if (f == Image::OSG_I_PF) return 1;
    if (f == Image::OSG_L_PF) return 1;
    if (f == Image::OSG_DEPTH_PF) return 1;
    if (f == Image::OSG_ALPHA_INTEGER_PF) return 1;
    if (f == Image::OSG_LUMINANCE_INTEGER_PF) return 1;
    if (f == Image::OSG_LA_PF) return 2;
    if (f == Image::OSG_LUMINANCE_ALPHA_INTEGER_PF) return 2;
    if (f == Image::OSG_RGB_PF) return 3;
    if (f == Image::OSG_BGR_PF) return 3;
    if (f == Image::OSG_RGB_INTEGER_PF) return 3;
    if (f == Image::OSG_BGR_INTEGER_PF) return 3;
    if (f == Image::OSG_RGBA_PF) return 4;
    if (f == Image::OSG_BGRA_PF) return 4;
    if (f == Image::OSG_RGBA_INTEGER_PF) return 4;
    if (f == Image::OSG_BGRA_INTEGER_PF) return 4;
    cout << "Warning: VRTexture::getChannels, unknown pixel format " << f << endl;
    return 0;
}

int VRTexture::getPixelByteN() {
    if (!img) return 0;
    auto f = img->getDataType();
    if (f == Image::OSG_INVALID_IMAGEDATATYPE) return 0;
    if (f == Image::OSG_UINT8_IMAGEDATA) return 1;
    if (f == Image::OSG_UINT16_IMAGEDATA) return 2;
    if (f == Image::OSG_UINT32_IMAGEDATA) return 4;
    if (f == Image::OSG_FLOAT16_IMAGEDATA) return 2;
    if (f == Image::OSG_FLOAT32_IMAGEDATA) return 4;
    if (f == Image::OSG_INT16_IMAGEDATA) return 2;
    if (f == Image::OSG_INT32_IMAGEDATA) return 4;
    if (f == Image::OSG_UINT24_8_IMAGEDATA) return 3;
    return 0;
}

string VRTexture::typeToString(int f) {
    if (f == Image::OSG_INVALID_IMAGEDATATYPE) return "OSG_INVALID_IMAGEDATATYPE";
    if (f == Image::OSG_UINT8_IMAGEDATA) return "OSG_UINT8_IMAGEDATA";
    if (f == Image::OSG_UINT16_IMAGEDATA) return "OSG_UINT16_IMAGEDATA";
    if (f == Image::OSG_UINT32_IMAGEDATA) return "OSG_UINT32_IMAGEDATA";
    if (f == Image::OSG_FLOAT16_IMAGEDATA) return "OSG_FLOAT16_IMAGEDATA";
    if (f == Image::OSG_FLOAT32_IMAGEDATA) return "OSG_FLOAT32_IMAGEDATA";
    if (f == Image::OSG_INT16_IMAGEDATA) return "OSG_INT16_IMAGEDATA";
    if (f == Image::OSG_INT32_IMAGEDATA) return "OSG_INT32_IMAGEDATA";
    if (f == Image::OSG_UINT24_8_IMAGEDATA) return "OSG_UINT24_8_IMAGEDATA";
    return "Unknown";
}

string VRTexture::formatToString(int f) {
#ifdef __EMSCRIPTEN__
    if (f == Image::OSG_R_PF) return "OSG_R_PF";
#else
    if (f == GL_RED) return "OSG_R_PF";
#endif
    if (f == Image::OSG_A_PF) return "OSG_A_PF";
    if (f == Image::OSG_I_PF) return "OSG_I_PF";
    if (f == Image::OSG_L_PF) return "OSG_L_PF";
    if (f == Image::OSG_DEPTH_PF) return "OSG_DEPTH_PF";
    if (f == Image::OSG_ALPHA_INTEGER_PF) return "OSG_ALPHA_INTEGER_PF";
    if (f == Image::OSG_LUMINANCE_INTEGER_PF) return "OSG_LUMINANCE_INTEGER_PF";
    if (f == Image::OSG_LA_PF) return "OSG_LA_PF";
    if (f == Image::OSG_LUMINANCE_ALPHA_INTEGER_PF) return "OSG_LUMINANCE_ALPHA_INTEGER_PF";
    if (f == Image::OSG_RGB_PF) return "OSG_RGB_PF";
    if (f == Image::OSG_RGB_INTEGER_PF) return "OSG_RGB_INTEGER_PF";
    if (f == Image::OSG_BGR_INTEGER_PF) return "OSG_BGR_INTEGER_PF";
    if (f == Image::OSG_RGBA_PF) return "OSG_RGBA_PF";
    if (f == Image::OSG_RGBA_INTEGER_PF) return "OSG_RGBA_INTEGER_PF";
    if (f == Image::OSG_BGRA_INTEGER_PF) return "OSG_BGRA_INTEGER_PF";
    return "Unknown";
}

int VRTexture::getPixelByteSize() { return getPixelByteN()*getChannels(); }

size_t VRTexture::getByteSize() {
    auto s = getSize();
    return s[0]*s[1]*s[2]*getPixelByteSize();
}

void VRTexture::setPixel(Vec3i p, Color4f c) {
    int w = img->getWidth();
    int h = img->getHeight();
    clampToImage(p);
    int i = p[0] + p[1]*w + p[2]*w*h;
    setIthPixel(i, c);
}

size_t VRTexture::getNPixel() {
    Vec3i s = getSize();
    return s[0]*s[1]*s[2];
}

void VRTexture::setIthPixel(int i, Color4f c) {
    int N = getChannels();
    int Nbytes = getPixelByteN();
    auto data = img->editData();

    if (N == 1) {
        if (Nbytes == 4) {
            float* f = (float*)data;
            f[i] = c[0];
        } else if (Nbytes == 1) {
            unsigned char* ub = (unsigned char*)data;
            ub[i] = c[0]*255;
        }
    }

    if (N == 2) {
        if (Nbytes == 4) {
            Vec2f* data2 = (Vec2f*)data;
            data2[i] = Vec2f(c[0], c[1]);
        } else if (Nbytes == 1) {
            Vec2ub* data2 = (Vec2ub*)data;
            data2[i] = Vec2ub(c[0]*255, c[1]*255);
        }
    }

    if (N == 3) {
        if (Nbytes == 4) {
            Color3f* data3 = (Color3f*)data;
            data3[i] = Color3f(c[0], c[1], c[2]);
        } else if (Nbytes == 1) {
            Color3ub* data3 = (Color3ub*)data;
            data3[i] = Color3ub(c[0]*255, c[1]*255, c[2]*255);
        }
    }

    if (N == 4) {
        if (Nbytes == 4) {
            Color4f* data4 = (Color4f*)data;
            data4[i] = c;
        } else if (Nbytes == 1) {
            Color4ub* data4 = (Color4ub*)data;
            data4[i] = Color4ub(c[0]*255, c[1]*255, c[2]*255, c[3]*255);
        }
    }
}

void VRTexture::clampToImage(Vec3i& p) {
    if (!img) return;
    if (p[0] < 0) p[0] = 0;
    if (p[0] >= img->getWidth()) p[0] = img->getWidth()-1;
    if (p[1] < 0) p[1] = 0;
    if (p[1] >= img->getHeight()) p[1] = img->getHeight()-1;
    if (p[2] < 0) p[2] = 0;
    if (p[2] >= img->getDepth()) p[2] = img->getDepth()-1;
}

vector<Color4f> VRTexture::getPixels(bool invertY) {
    vector<Color4f> res;
    size_t N = getNPixel();
    for (size_t i=0; i<N; i++) res.push_back(getPixel(i, invertY));
    return res;
}

Color4f VRTexture::getPixelVec(Vec3i p, bool invertY) {
    auto res = Color4f(0,0,0,1);
    if (!img) return res;
    //int N = getChannels();
    int w = img->getWidth();
    int h = img->getHeight();

    //auto data = img->getData();
    clampToImage(p);
    int i = p[0] + p[1]*w + p[2]*w*h;

    return getPixel(i, invertY);
}

Color4f VRTexture::getPixelUV(Vec2d uv, bool invertY) {
    auto res = Color4f(0,0,0,1);
    if (!img) return res;
    int w = img->getWidth();
    int h = img->getHeight();
    int x = uv[0]*(w-1);
    int y = uv[1]*(h-1);
    return getPixelVec(Vec3i(x,y,0), invertY);
}

Color4f VRTexture::getPixel(int i, bool invertY) {
    auto res = Color4f(0,0,0,1);
    if (!img) return res;
    int N = getChannels();
    //int pbN = getPixelByteN();
    auto f = img->getDataType();
    auto data = img->getData();
    auto s = getSize();
    size_t S = s[0]*s[1]*s[2];
    bool valid = bool(i >= 0 && i < (int)S);
    if (!valid) return Color4f();

    if (invertY) { // TODO: extend for 3D texture
        int x = i%s[0];
        int y = i/s[0];
        y = s[1]-1 - y;
        i = x + y*s[0];
    }

    //cout << "VRTexture::getPixel " << i << ", " << N << ", " << f << endl;

    if (N == 1) {
        float d = 0;

        if (f == Image::OSG_UINT8_IMAGEDATA)   d = ((UInt8*)data)[i]/256.0;
        if (f == Image::OSG_INT16_IMAGEDATA)   d = ((Int16*)data)[i]/32768.0;
        if (f == Image::OSG_UINT16_IMAGEDATA)  d = ((UInt16*)data)[i]/65536.0;
        if (f == Image::OSG_FLOAT16_IMAGEDATA) d = ((Real16*)data)[i];
        if (f == Image::OSG_INT32_IMAGEDATA)   d = ((Int32*)data)[i]/2147483648.0;
        if (f == Image::OSG_UINT32_IMAGEDATA)  d = ((UInt32*)data)[i]/4294967296.0;
        if (f == Image::OSG_FLOAT32_IMAGEDATA) d = ((Real32*)data)[i];

        res = Color4f(d, d, d, 1.0);
    }

    if (N == 2) {
        Vec2f d;

        if (f == Image::OSG_UINT8_IMAGEDATA)   for (int j=0; j<2; j++) d[j] = ((UInt8*)data)[2*i+j]/256.0;
        if (f == Image::OSG_INT16_IMAGEDATA)   for (int j=0; j<2; j++) d[j] = ((Int16*)data)[2*i+j]/32768.0;
        if (f == Image::OSG_UINT16_IMAGEDATA)  for (int j=0; j<2; j++) d[j] = ((UInt16*)data)[2*i+j]/65536.0;
        if (f == Image::OSG_FLOAT16_IMAGEDATA) for (int j=0; j<2; j++) d[j] = ((Real16*)data)[2*i+j];
        if (f == Image::OSG_INT32_IMAGEDATA)   for (int j=0; j<2; j++) d[j] = ((Int32*)data)[2*i+j]/2147483648.0;
        if (f == Image::OSG_UINT32_IMAGEDATA)  for (int j=0; j<2; j++) d[j] = ((UInt32*)data)[2*i+j]/4294967296.0;
        if (f == Image::OSG_FLOAT32_IMAGEDATA) for (int j=0; j<2; j++) d[j] = ((Real32*)data)[2*i+j];

        res = Color4f(d[0], d[1], 0.0, 1.0);
    }

    if (N == 3) {
        Color3f d;

        if (f == Image::OSG_UINT8_IMAGEDATA)   for (int j=0; j<3; j++) d[j] = ((UInt8*)data)[3*i+j]/256.0;
        if (f == Image::OSG_INT16_IMAGEDATA)   for (int j=0; j<3; j++) d[j] = ((Int16*)data)[3*i+j]/32768.0;
        if (f == Image::OSG_UINT16_IMAGEDATA)  for (int j=0; j<3; j++) d[j] = ((UInt16*)data)[3*i+j]/65536.0;
        if (f == Image::OSG_FLOAT16_IMAGEDATA) for (int j=0; j<3; j++) d[j] = ((Real16*)data)[3*i+j];
        if (f == Image::OSG_INT32_IMAGEDATA)   for (int j=0; j<3; j++) d[j] = ((Int32*)data)[3*i+j]/2147483648.0;
        if (f == Image::OSG_UINT32_IMAGEDATA)  for (int j=0; j<3; j++) d[j] = ((UInt32*)data)[3*i+j]/4294967296.0;
        if (f == Image::OSG_FLOAT32_IMAGEDATA) for (int j=0; j<3; j++) d[j] = ((Real32*)data)[3*i+j];

        res = Color4f(d[0], d[1], d[2], 1.0);
    }

    if (N == 4) {
        Color4f d;

        if (f == Image::OSG_UINT8_IMAGEDATA)   for (int j=0; j<4; j++) d[j] = ((UInt8*)data)[4*i+j]/256.0;
        if (f == Image::OSG_INT16_IMAGEDATA)   for (int j=0; j<4; j++) d[j] = ((Int16*)data)[4*i+j]/32768.0;
        if (f == Image::OSG_UINT16_IMAGEDATA)  for (int j=0; j<4; j++) d[j] = ((UInt16*)data)[4*i+j]/65536.0;
        if (f == Image::OSG_FLOAT16_IMAGEDATA) for (int j=0; j<4; j++) d[j] = ((Real16*)data)[4*i+j];
        if (f == Image::OSG_INT32_IMAGEDATA)   for (int j=0; j<4; j++) d[j] = ((Int32*)data)[4*i+j]/2147483648.0;
        if (f == Image::OSG_UINT32_IMAGEDATA)  for (int j=0; j<4; j++) d[j] = ((UInt32*)data)[4*i+j]/4294967296.0;
        if (f == Image::OSG_FLOAT32_IMAGEDATA) for (int j=0; j<4; j++) d[j] = ((Real32*)data)[4*i+j];

        res = d;
    }

    return res;
}

Vec3i VRTexture::getSize() {
    if (!img) return Vec3i();
    int w = img->getWidth();
    int h = img->getHeight();
    int d = img->getDepth();
    return Vec3i(w,h,d);
}

float VRTexture::getAspectRatio() {
    int W = img->getWidth();
    int H = img->getHeight();
    if (H == 0) cout << "Warning in VRTexture::getAspectRatio! image height is " << H << endl;
    return H > 0 ? float(W)/H : 1;
}

void VRTexture::downsize() {
    //auto des = VRTexture::create(img);

    ImageMTRecPtr nimg = Image::create();
    if (!img) return;
    int N = getChannels();
    Vec3i s = getSize();
    s[0] = int(0.5*s[0]);
    s[1] = int(0.5*s[1]);
    s[2] = int(0.5*s[2]);
    if (s[0] == 0) s[0] = 1;
    if (s[1] == 0) s[1] = 1;
    if (s[2] == 0) s[2] = 1;
        //cout << " tex size " << s << endl;
    nimg->set(img->getPixelFormat(), s[0], s[1], s[2],
              img->getMipMapCount(), img->getFrameCount(), img->getFrameDelay(),
              0, img->getDataType(), true, img->getSideCount());

    if ( N == 1 || N == 3 ) {
        auto f = img->getDataType();
        auto data = nimg->editData();
        int N = getChannels();
        int w = s[0];
        int h = s[1];
        for (int z = 0; z < s[2]; z++) {
            for (int y = 0; y < s[1]; y++) {
                for (int x = 0; x < s[0]; x++) {
                    Color4f p1 = getPixelVec(Vec3i(2*x,2*y,2*z));
                    Color4f p2 = getPixelVec(Vec3i(2*x+1,2*y,2*z));
                    Color4f p3 = getPixelVec(Vec3i(2*x,2*y+1,2*z));
                    Color4f p4 = getPixelVec(Vec3i(2*x+1,2*y+1,2*z));
                    Color4f p5 = getPixelVec(Vec3i(2*x,2*y,2*z+1));
                    Color4f p6 = getPixelVec(Vec3i(2*x+1,2*y,2*z+1));
                    Color4f p7 = getPixelVec(Vec3i(2*x,2*y+1,2*z+1));
                    Color4f p8 = getPixelVec(Vec3i(2*x+1,2*y+1,2*z+1));
                    Color4f newC = Color4f((p1+p2+p3+p4+p5+p6+p7+p8)*0.125);
                    int i = N*(x + y*w + z*w*h);

                    if ( N == 1 ) {
                        if (f == Image::OSG_FLOAT32_IMAGEDATA) {
                            if (i >= s[0]*s[1]*s[2]) cout << "AAAAAAAAAA " << i << "/" << s[0]*s[1]*s[2] << "   s: " << s << endl;
                            else {
                                float* f = (float*)data;
                                f[i] = newC[0];
                            }
                        }
                    }

                    if ( N == 3 ) {
                        if (f == Image::OSG_UINT8_IMAGEDATA) {
                            if ( i >= N*(s[0]*s[1]*s[2]) ) cout << "AAAAAAAAAA " << i << "/" << N*(s[0]*s[1]*s[2]) << "   s: " << s << endl;
                            else {
                                newC *= 256.0;
                                //if (x == 0) cout << newC << endl;
                                char* f = (char*)data;
                                f[i] = newC[0];
                                f[i+1] = newC[1];
                                f[i+2] = newC[2];
                            }
                        }
                    }
                }
            }
        }
        img = nimg;
    }
    if ( N == 2 ) { }
    if ( N == 4 ) { }
}

void VRTexture::mixColor(Color4f c, float a) {
    size_t N = getNPixel();
    for (size_t i=0; i<N; i++) {
        Color4f p = getPixel(i);
        setIthPixel(i, p*(1-a) + c*a);
    }
}

