#include "VRSprite.h"

#include "core/tools/VRText.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRGeoData.h"
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
}

VRSprite::~VRSprite() {}

VRSpritePtr VRSprite::create(string name, bool alpha, float w, float h) {
    auto s = shared_ptr<VRSprite>(new VRSprite(name, alpha, w, h) );
    s->updateGeo();
    return s;
}

VRSpritePtr VRSprite::ptr() { return static_pointer_cast<VRSprite>( shared_from_this() ); }

void VRSprite::updateGeo() {
    VRGeoData data;
    float w2 = width*0.5;
    float h2 = height*0.5;
    data.pushVert(Pnt3d(-w2,h2,0), Vec3d(0,0,1), Vec2d(0,1));
    data.pushVert(Pnt3d(-w2,-h2,0), Vec3d(0,0,1), Vec2d(0,0));
    data.pushVert(Pnt3d(w2,-h2,0), Vec3d(0,0,1), Vec2d(1,0));
    data.pushVert(Pnt3d(w2,h2,0), Vec3d(0,0,1), Vec2d(1,1));
    data.pushQuad();
    data.apply(ptr());
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
    VRDevicePtr multitouch = VRSetup::getCurrent()->getDevice("multitouch");

    web->setMaterial(mat);
    web->open(path);
    if (mouse) web->addMouse(mouse, ptr(), 0, 2, 3, 4);
    if (flystick) web->addMouse(flystick, ptr(), 0, -1, -1, -1);
    if (multitouch) web->addMouse(multitouch, ptr(), 0, 2, 3, 4);
    if (keyboard) web->addKeyboard(keyboard);
    web->setResolution(res);
    web->setAspectRatio(ratio);
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
Vec2d VRSprite::getSize() { return Vec2d(width, height); }

void VRSprite::setSize(float w, float h) {
    width = w;
    height = h;
    updateGeo();
}

void VRSprite::convertToCloth() {
    getPhysics()->setDynamic(true);
    getPhysics()->setShape("Cloth");
    getPhysics()->setSoft(true);
    getPhysics()->setPhysicalized(true);
}

OSG_END_NAMESPACE;
