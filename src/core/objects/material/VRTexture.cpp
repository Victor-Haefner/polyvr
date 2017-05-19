#include "VRTexture.h"
#include "core/math/boundingbox.h"

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

void VRTexture::read(string path) {
    if (!img->read(path.c_str())) cout << " VRTexture::read failed!" << endl;
}

void VRTexture::write(string path) {
    if (!img->write(path.c_str())) cout << " VRTexture::write failed!" << endl;
}

void VRTexture::paste(VRTexturePtr other, Vec3i offset) {
    const UInt8* data = other->getImage()->getData();
    Vec3i s1 = getSize();
    Vec3i s2 = other->getSize();
    if (s2[0]*s2[1]*s2[2] == 0) return;

    Vec3i S = offset + s2;
    if (S[0] > s1[0]) { cout << "VRTexture::paste sx too big: " << S << " s " << s1 << endl; return; }
    if (S[1] > s1[1]) { cout << "VRTexture::paste sy too big: " << S << " s " << s1 << endl; return; }
    if (S[2] > s1[2]) { cout << "VRTexture::paste sz too big: " << S << " s " << s1 << endl; return; }

    auto data1 = img->editData();
    auto data2 = other->img->editData();
    int Bpp = getPixelByteSize();
    for (int k=0; k<s2[2]; k++) {
        for (int j=0; j<s2[1]; j++) {
            size_t J1 = (offset[0] + (j+offset[1])*s1[0] + (k+offset[2])*s1[0]*s1[1])*Bpp;
            size_t J2 = (j*s2[0] + k*s2[0]*s2[1])*Bpp;
            memcpy(data1+J1, data2+J2, s2[0]*Bpp);
        }
    }

    //img->setSubData(offset[0], offset[1], offset[2], dim[0], dim[1], dim[2], data);
    // TODO: evaluate the speed against the setSubData function!
}

void VRTexture::resize(Vec3i size, Vec3i offset) {
    auto tmp = VRTexture::create(img);
    vector<char> data( getByteSize(), 0 );
    ImageRecPtr nimg = Image::create();
    nimg->set(img->getPixelFormat(), size[0], size[1], size[2],
              img->getMipMapCount(), img->getFrameCount(), img->getFrameDelay(),
              (const uint8_t*)&data[0], img->getDataType(), true, img->getSideCount());
    img = nimg;
    paste(tmp, offset);
}

void VRTexture::merge(VRTexturePtr other, Vec3f pos) {
    if (!other) return;
    if (!other->img) return;
    if (!img || getSize()[0] == 0) { img = other->img; return; }

    Vec3i dim1 = getSize();
    Vec3i dim2 = other->getSize();
    Vec3i offset = Vec3i(dim1[0]*pos[0], dim1[1]*pos[1], dim1[2]*pos[2]);
    Vec3i offset2 = Vec3i(min(0,offset[0]), min(0,offset[1]), min(0,offset[2]));

    boundingbox bb; // compute total new size
    bb.update( { Vec3f(offset), Vec3f(offset2), Vec3f(offset+dim2), Vec3f(offset2+dim1) } );
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

Vec4f VRTexture::getPixel(Vec2f uv) { // TODO: check data format (float/integer/char)
    auto res = Vec4f(0,0,0,1);
    if (!img) return res;
    int N = getChannels();
    //int B = img->getBpp()/N;
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

Vec3i VRTexture::getSize() {
    int w = img->getWidth();
    int h = img->getHeight();
    int d = img->getDepth();
    return Vec3i(w,h,d);
}




