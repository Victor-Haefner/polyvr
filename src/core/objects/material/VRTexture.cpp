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
    Vec3i dim = other->getSize();
    if (dim[0]*dim[1]*dim[2] == 0) return;
    img->setSubData(offset[0], offset[1], offset[2], dim[0], dim[1], dim[2], data);
}

void VRTexture::resize(Vec3i size, Vec3i offset) {
    auto tmp = VRTexture::create(img);
    ImageRecPtr nimg = Image::create();
    vector<char> data( img->getBpp()/8, 0 );
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




