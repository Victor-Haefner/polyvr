#include "VRSprite.h"

#include "core/tools/VRText.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/setup/VRSetup.h"
#include "addons/CEF/CEF.h"
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGGeoProperties.h>
#include <sstream>


OSG_BEGIN_NAMESPACE;
using namespace std;

VRSprite::VRSprite (string name, bool alpha, float w, float h) : VRGeometry(name) {
    width = w;
    height = h;
    type = "Sprite";
    fontColor = Color4f(0,0,0,255);
    backColor = Color4f(0,0,0,0);
    updateGeo();
}

VRSprite::~VRSprite() {}

VRSpritePtr VRSprite::create(string name, bool alpha, float w, float h) { return shared_ptr<VRSprite>(new VRSprite(name, alpha, w, h) ); }
VRSpritePtr VRSprite::ptr() { return static_pointer_cast<VRSprite>( shared_from_this() ); }

void VRSprite::updateGeo() {
    //setMesh(makePlaneGeo(width, height, 1, 1));
    GeoPnt3fPropertyRecPtr      pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr      norms = GeoVec3fProperty::create();
    GeoVec2fPropertyRefPtr      texs = GeoVec2fProperty::create();
    GeoUInt32PropertyRecPtr     inds = GeoUInt32Property::create();

    float w2 = width*0.5;
    float h2 = height*0.5;
    pos->addValue(Pnt3f(-w2,h2,0));
    pos->addValue(Pnt3f(-w2,-h2,0));
    pos->addValue(Pnt3f(w2,-h2,0));
    pos->addValue(Pnt3f(w2,h2,0));

    texs->addValue(Vec2f(0,1));
    texs->addValue(Vec2f(0,0));
    texs->addValue(Vec2f(1,0));
    texs->addValue(Vec2f(1,1));

    for (int i=0; i<4; i++) {
        norms->addValue(Vec3f(0,0,1));
        inds->addValue(i);
    }

    setType(GL_QUADS);
    setPositions(pos);
    setNormals(norms);
    setTexCoords(texs);
    setIndices(inds);
}

void VRSprite::setLabel (string l, float res) {
    if (l == label) return;
    label = l;
    auto tex = VRText::get()->create(l, font, 20*res, fontColor, backColor );
    //labelMat->setSortKey(1);
    getMaterial()->setTexture(tex);
}

void VRSprite::webOpen(string path, int res, float ratio){
    VRMaterialPtr mat = VRMaterial::get(getName()+"web");
    setMaterial(mat);
    mat->setLit(false);
    web = CEF::create();

    VRDevicePtr mouse = VRSetup::getCurrent()->getDevice("mouse");
    VRDevicePtr keyboard = VRSetup::getCurrent()->getDevice("keyboard");
    VRDevicePtr flystick = VRSetup::getCurrent()->getDevice("flystick");

    web->setMaterial(mat);
    web->open(path);
    if (mouse) web->addMouse(mouse, ptr(), 0, 2, 3, 4);
    if (flystick) web->addMouse(flystick, ptr(), 0, -1, -1, -1);
    if (keyboard) web->addKeyboard(keyboard);
    web->setResolution(res);
    web->setAspectRatio(ratio);

    // flip normals for deferred shading
    auto norms = getMesh()->geo->getNormals();
    for (uint i=0; i<norms->size(); i++) {
        Vec3f n = norms->getValue<Vec3f>(i);
        norms->setValue(-n, i);
    }
}

void VRSprite::setTexture(string path) {
    auto m = VRMaterial::create(path);
    m->setTexture(path);
    setMaterial(m);
}

void VRSprite::setFont(string f) { font = f; }

void VRSprite::setFontColor(Color4f c) { fontColor = c; }
void VRSprite::setBackColor(Color4f c) { backColor = c; }

string VRSprite::getLabel() { return label; }
Vec2f VRSprite::getSize() { return Vec2f(width, height); }

void VRSprite::setSize(float w, float h) {
    width = w;
    height = h;
    updateGeo();
}

OSG_END_NAMESPACE;
