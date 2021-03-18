#include "VRColorChooser.h"
#include "core/objects/material/VRShader.h"
#include "core/setup/devices/VRDevice.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGImage.h>

using namespace OSG;


VRColorChooser::VRColorChooser() {
    mat = VRMaterial::create("color choose");
    mat->setLit(false);
    setColor(Color3f(1,1,1));
}

VRColorChooser::~VRColorChooser() {}

shared_ptr<VRColorChooser> VRColorChooser::create() { return shared_ptr<VRColorChooser>(new VRColorChooser()); }

Color3f VRColorChooser::colFromUV(Vec2d tc) {
    if (abs(tc[0]-0.5) > 0.5-border || abs(tc[1]-0.5) > 0.5-border) return color;

    tc *= 1.4;
    tc -= Vec2d(0.2, 0.2);
    Color3f c;
    Color3f::convertFromHSV(&c[0], 255*tc[0], tc[1], 1);
    return c;
}

void VRColorChooser::updateTexture() {
    ImageMTRecPtr img = Image::create();
    int s = 128;

    Color3f c = getColor();

    Color3f* data = new Color3f[s*s];
    for (int i=0; i<s; i++) {
        for (int j=0; j<s; j++) {
            Vec2d tc = Vec2d(j, i)*1.0/s;
            data[i*s+j] = colFromUV(tc);
        }
    }

    img->set( Image::OSG_RGB_PF, s, s, 1, 0, 1, 0, (const uint8_t*)data, OSG::Image::OSG_FLOAT32_IMAGEDATA, true, 1);
    mat->setTexture(VRTexture::create(img));
}

void VRColorChooser::setColor(Color3f c) { last_color = color; color = c; updateTexture(); }
Color3f VRColorChooser::getColor() { return color; }
Color3f VRColorChooser::getLastColor() { return last_color; }

void VRColorChooser::setGeometry(VRGeometryPtr g) { if (!g) return; geo = g; g->setMaterial(mat); }

void VRColorChooser::resolve(VRDevicePtr dev) {
    if (dev == 0) return;

    //VRIntersection ins = dev->getLastIntersection();
    VRIntersection ins = dev->intersect(geo);
    if (!ins.hit) return;
    auto obj = ins.object.lock();
    auto g = geo.lock();
    if (obj != g || !g || !obj) return;

    setColor(colFromUV(ins.texel));
}

