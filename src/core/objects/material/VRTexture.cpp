#include "VRTexture.h"

#include <OpenSG/OSGImage.h>

using namespace OSG;

VRTexture::VRTexture() { img = Image::create(); }
VRTexture::VRTexture(ImageRecPtr img) { this->img = img; }
VRTexture::~VRTexture() {}

VRTexturePtr VRTexture::create() { return shared_ptr<VRTexture>(new VRTexture() ); }
VRTexturePtr VRTexture::create(ImageRecPtr img) { return shared_ptr<VRTexture>(new VRTexture(img) ); }
VRTexturePtr VRTexture::ptr() { return shared_from_this(); }


void VRTexture::setImage(ImageRecPtr img) { this->img = img; }
void VRTexture::setInternalFormat(int ipf) { internal_format = ipf; }
int VRTexture::getInternalFormat() { return internal_format; }
ImageRecPtr VRTexture::getImage() { return img; }

void VRTexture::read(string path) { img->read(path.c_str()); }
void VRTexture::write(string path) { img->write(path.c_str()); }

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

/*int VRTexture::getByteSize() {
    if (!img) return 0;
    auto f = img->getPixelFormat();
    Image::
    if (OSG_INVALID_IMAGEDATATYPE  = GL_NONE,
        OSG_UINT8_IMAGEDATA        = GL_UNSIGNED_BYTE,
        OSG_UINT16_IMAGEDATA       = GL_UNSIGNED_SHORT,
        OSG_UINT32_IMAGEDATA       = GL_UNSIGNED_INT,
        OSG_FLOAT16_IMAGEDATA      = GL_HALF_FLOAT_NV,
        OSG_FLOAT32_IMAGEDATA      = GL_FLOAT,
        OSG_INT16_IMAGEDATA        = GL_SHORT,
        OSG_INT32_IMAGEDATA        = GL_INT,
        OSG_UINT24_8_IMAGEDATA     = GL_NONE
    return 0;
}*/

Vec4f VRTexture::getPixel(Vec2f uv) { // TODO: check data format (float/integer/char)
    auto res = Vec4f(0,0,0,1);
    if (!img) return res;
    int N = getChannels();
    int B = img->getBpp()/N;
    int w = img->getWidth();
    int h = img->getHeight();
    int x = uv[0]*(w-1);
    int y = uv[1]*(h-1);
    auto data = img->getData();
    int i = x + y*w;

    if (N == 1) {
        float* f = (float*)data;
        float d = f[i];
        res = Vec4f(d, d, d, 1.0);
    }

    if (N == 2) {
        Vec2f* data2 = (Vec2f*)data;
        Vec2f d = data2[i];
        res = Vec4f(d[0], d[1], 0.0, 1.0);
    }

    if (N == 3) {
        Vec3f* data3 = (Vec3f*)data;
        Vec3f d = data3[i];
        res = Vec4f(d[0], d[1], d[2], 1.0);
    }

    if (N == 4) {
        Vec4f* data4 = (Vec4f*)data;
        res = data4[i];
    }

    //cout << "VRTexture::getPixel " << Vec4i(B, N, w, h) << " uv " << uv << " i " << i << " xy " << Vec2i(x,y) <<  " N " << w*h << " c " << res << endl;
    //cout << "  size " << img->getSize();
    return res;
}


