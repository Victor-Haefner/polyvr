#include "VRColorChooser.h"
#include "core/objects/material/VRShader.h"
#include "core/setup/devices/VRDevice.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"

#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGGeoProperties.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRColorChooser::VRColorChooser() {
    mat = VRMaterial::create("color choose");
    mat->setLit(false);
    setColor(Vec3f(1,1,1));
}

Color3f VRColorChooser::colFromUV(Vec2f tc) {
    if (abs(tc[0]-0.5) > 0.5-border || abs(tc[1]-0.5) > 0.5-border) return color;

    tc *= 1.4;
    tc -= Vec2f(0.2, 0.2);
    Color3f c;
    Color3f::convertFromHSV(&c[0], 255*tc[0], tc[1], 1);
    return c;
}

void VRColorChooser::updateTexture() {
    ImageRecPtr img = Image::create();
    int s = 128;

    Color3f c = getColor();

    Color3f* data = new Color3f[s*s];
    for (int i=0; i<s; i++) {
        for (int j=0; j<s; j++) {
            Vec2f tc = Vec2f(j, i)*1.0/s;
            data[i*s+j] = colFromUV(tc);
        }
    }

    img->set( Image::OSG_RGB_PF, s, s, 1, 0, 1, 0, (const uint8_t*)data, OSG::Image::OSG_FLOAT32_IMAGEDATA, true, 1);
    mat->setTexture(img);
}

void VRColorChooser::setColor(Color3f c) { last_color = color; color = c; updateTexture(); }
Color3f VRColorChooser::getColor() { return color; }
Color3f VRColorChooser::getLastColor() { return last_color; }

void VRColorChooser::setGeometry(VRGeometryPtr g) { geo = g; geo->setMaterial(mat); }

void VRColorChooser::resolve(VRDevice* dev) {
    if (dev == 0) return;

    //VRIntersection ins = dev->getLastIntersection();
    VRIntersection ins = dev->intersect(geo);
    if (!ins.hit) return;
    if (ins.object == 0) return;
    if (ins.object != geo) return;

    cout << "VRColorChooser::resolve, geo: " << geo->getName() << " texel: " << ins.texel << endl;

    setColor(colFromUV(ins.texel));
}

OSG_END_NAMESPACE;
