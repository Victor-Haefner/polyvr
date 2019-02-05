#include "VRTexture.h"
#include "core/math/boundingbox.h"
#include "core/utils/toString.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRScene.h"

#include <OpenSG/OSGImage.h>
#include <boost/bind.hpp>

using namespace OSG;

template<> string typeName(const VRTexturePtr& t) { return "Texture"; }

VRTexture::VRTexture() { img = Image::create(); }
VRTexture::VRTexture(ImageMTRecPtr img) { this->img = img; }
VRTexture::~VRTexture() {}

VRTexturePtr VRTexture::create() { return shared_ptr<VRTexture>(new VRTexture() ); }
VRTexturePtr VRTexture::create(ImageMTRecPtr img) { return shared_ptr<VRTexture>(new VRTexture(img) ); }
VRTexturePtr VRTexture::ptr() { return shared_from_this(); }


void VRTexture::setImage(ImageMTRecPtr img) { this->img = img; }
void VRTexture::setInternalFormat(int ipf) { internal_format = ipf; }
int VRTexture::getInternalFormat() { return internal_format; }
ImageMTRecPtr VRTexture::getImage() { return img; }

void VRTexture::read(string path) {
    if (!img->read(path.c_str())) cout << " VRTexture::read from file '" << path << "' failed!" << endl;
}

void VRTexture::writeThreaded(string path, VRTexturePtr self, VRThreadWeakPtr tw) { // TODO: add mutex!
    string folder = getFolderName(path);
    if (!exists(folder)) makedir(folder);
    if (!img->write(path.c_str())) cout << " VRTexture::write to file '" << path << "' failed!" << endl;
    self.reset();
    writeWorker.reset();
}

void VRTexture::write(string path, bool doThread) {
    if (!doThread) {
        string folder = getFolderName(path);
        if (!exists(folder)) makedir(folder);
        if (!img->write(path.c_str())) cout << " VRTexture::write to file '" << path << "' failed!" << endl;
    } else {
        auto scene = VRScene::getCurrent();
        if (!writeWorker) writeWorker = VRThreadCb::create("writeThreaded", boost::bind(&VRTexture::writeThreaded, this, path, ptr(), _1));
        scene->initThread(writeWorker, "write image to disk");
    }
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
    if (S[0] > s1[0]) { cout << "VRTexture::paste sx too big: " << S << " s " << s1 << endl; return; }
    if (S[1] > s1[1]) { cout << "VRTexture::paste sy too big: " << S << " s " << s1 << endl; return; }
    if (S[2] > s1[2]) { cout << "VRTexture::paste sz too big: " << S << " s " << s1 << endl; return; }


    auto data1 = img->editData();
    auto data2 = other->img->editData();
    int Bpp1 = getPixelByteSize();
    int Bpp2 = other->getPixelByteSize();
    int BppMin = min(Bpp1, Bpp2);
    for (int k=0; k<s2[2]; k++) {
        for (int j=0; j<s2[1]; j++) {
            size_t J1 = (offset[0] + (j+offset[1])*s1[0] + (k+offset[2])*s1[0]*s1[1])*Bpp1;
            size_t J2 = (j*s2[0] + k*s2[0]*s2[1])*Bpp2;
            if (Bpp1 == Bpp2) memcpy(data1+J1, data2+J2, s2[0]*Bpp2); // copy whole line
            else {
                for (int i=0; i<s2[0]; i++) {
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

void VRTexture::resize(Vec3i size, Vec3i offset) {
    auto tmp = VRTexture::create(img);
    ImageMTRecPtr nimg = Image::create();
    nimg->set(img->getPixelFormat(), size[0], size[1], size[2],
              img->getMipMapCount(), img->getFrameCount(), img->getFrameDelay(),
              0, img->getDataType(), true, img->getSideCount());
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
    resize(dim3, offset2);
    paste(other, offset);
}

int VRTexture::getChannels() {
    if (!img) return 0;
    auto f = img->getPixelFormat();
    if (f == Image::OSG_A_PF) return 1;
    if (f == Image::OSG_I_PF) return 1;
    if (f == Image::OSG_L_PF) return 1;
    if (f == Image::OSG_DEPTH_PF) return 1;
    if (f == Image::OSG_ALPHA_INTEGER_PF) return 1;
    if (f == Image::OSG_LUMINANCE_INTEGER_PF) return 1;
    if (f == Image::OSG_DEPTH_PF) return 1;
    if (f == Image::OSG_DEPTH_PF) return 1;
    if (f == Image::OSG_LA_PF) return 2;
    if (f == Image::OSG_LUMINANCE_ALPHA_INTEGER_PF) return 2;
    if (f == Image::OSG_RGB_PF) return 3;
    if (f == Image::OSG_RGB_INTEGER_PF) return 3;
    if (f == Image::OSG_BGR_INTEGER_PF) return 3;
    if (f == Image::OSG_RGBA_PF) return 4;
    if (f == Image::OSG_RGBA_INTEGER_PF) return 4;
    if (f == Image::OSG_BGRA_INTEGER_PF) return 4;
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

int VRTexture::getPixelByteSize() { return getPixelByteN()*getChannels(); }

size_t VRTexture::getByteSize() {
    auto s = getSize();
    return s[0]*s[1]*s[2]*getPixelByteSize();
}

void VRTexture::setPixel(Vec3i p, Color4f c) {
    int N = getChannels();
    int w = img->getWidth();
    int h = img->getHeight();

    auto data = img->editData();
    int i = p[0] + p[1]*w + p[2]*w*h;

    if (N == 1) {
        float* f = (float*)data;
        f[i] = c[0];
    }

    if (N == 2) {
        Vec2f* data2 = (Vec2f*)data;
        data2[i] = Vec2f(c[0], c[1]);
    }

    if (N == 3) {
        Color3f* data3 = (Color3f*)data;
        data3[i] = Color3f(c[0], c[1], c[2]);
    }

    if (N == 4) {
        Color4f* data4 = (Color4f*)data;
        data4[i] = c;
    }
}

void VRTexture::clampToImage(Vec3i& p) {
    if (!img) return;
    if (p[0] < 0) p[0] = 0; if (p[0] >= img->getWidth()) p[0] = img->getWidth()-1;
    if (p[1] < 0) p[1] = 0; if (p[1] >= img->getHeight()) p[1] = img->getHeight()-1;
    if (p[2] < 0) p[2] = 0; if (p[2] >= img->getDepth()) p[2] = img->getDepth()-1;
}

Color4f VRTexture::getPixel(Vec3i p) { // TODO: check data format (float/integer/char)
    auto res = Color4f(0,0,0,1);
    if (!img) return res;
    int N = getChannels();
    int w = img->getWidth();
    int h = img->getHeight();

    auto data = img->getData();
    clampToImage(p);
    int i = p[0] + p[1]*w + p[2]*w*h;

    return getPixel(i);
}

Color4f VRTexture::getPixel(Vec2d uv) {
    auto res = Color4f(0,0,0,1);
    if (!img) return res;
    int w = img->getWidth();
    int h = img->getHeight();
    int x = uv[0]*(w-1);
    int y = uv[1]*(h-1);
    return getPixel(Vec3i(x,y,0));
}

Color4f VRTexture::getPixel(int i) {
    auto res = Color4f(0,0,0,1);
    if (!img) return res;
    int N = getChannels();
    auto data = img->getData();

    if (N == 1) {
        float* f = (float*)data;
        float d = f[i];
        res = Color4f(d, d, d, 1.0);
    }

    if (N == 2) {
        Vec2f* data2 = (Vec2f*)data;
        Vec2f d = data2[i];
        res = Color4f(d[0], d[1], 0.0, 1.0);
    }

    if (N == 3) {
        Color3f* data3 = (Color3f*)data;
        Color3f d = data3[i];
        res = Color4f(d[0], d[1], d[2], 1.0);
    }

    if (N == 4) {
        Color4f* data4 = (Color4f*)data;
        res = data4[i];
    }

    return res;
}

Vec3i VRTexture::getSize() {
    int w = img->getWidth();
    int h = img->getHeight();
    int d = img->getDepth();
    return Vec3i(w,h,d);
}

float VRTexture::getAspectRatio() {
    return float(img->getWidth()) / img->getHeight();
}



