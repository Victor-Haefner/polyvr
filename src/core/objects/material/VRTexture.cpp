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

Vec4f VRTexture::getPixel(Vec2f uv) {
    auto res = Vec4f(0,0,0,1);
    if (!img) return res;
    auto data = img->getData();
    //return data[]; // TODO
    return res;
}
