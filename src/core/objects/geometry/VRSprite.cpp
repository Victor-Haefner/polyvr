#include "VRSprite.h"

#include "core/tools/VRText.h"
#include "core/objects/material/VRMaterial.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "addons/CEF/CEF.h"
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <sstream>


OSG_BEGIN_NAMESPACE;
using namespace std;

VRSprite::VRSprite (string name, bool alpha, float w, float h) : VRGeometry(name) {
    width = w;
    height = h;
    type = "Sprite";

    setMesh(makePlaneGeo(width, height, 1, 1));
    setAt(Vec3f(0,0,-1));

    font = "SANS 20";
    fontColor = Color4f(0,0,0,255);
    label = "";
}

VRSprite::~VRSprite() {}

VRSpritePtr VRSprite::create(string name, bool alpha, float w, float h) { return shared_ptr<VRSprite>(new VRSprite(name, alpha, w, h) ); }
VRSpritePtr VRSprite::ptr() { return static_pointer_cast<VRSprite>( shared_from_this() ); }

void VRSprite::setLabel (string l, float res) {
    if (l == label) return;
    label = l;
    auto labelMat = VRText::get()->getTexture(l, font, 20*res, fontColor, Color4f(0,0,0,0) );
    labelMat->setSortKey(1);
    setMaterial(labelMat);
}

void VRSprite::webOpen(string path, int res, float ratio){
    VRMaterialPtr mat = VRMaterial::get(getName()+"web");
    setMaterial(mat);
    mat->setLit(false);
    web = CEF::create();

    VRDevice* mouse = VRSetupManager::getCurrent()->getDevice("mouse");
    VRDevice* keyboard = VRSetupManager::getCurrent()->getDevice("keyboard");

    web->setMaterial(mat);
    web->open(path);
    web->addMouse(mouse, ptr(), 0, 2, 3, 4);
    web->addKeyboard(keyboard);
    web->setResolution(res);
    web->setAspectRatio(ratio);
}

void VRSprite::setTexture(string path){
    getMaterial()->setTexture(path);
    setMaterial(getMaterial());
}

void VRSprite::setFont(string f) { font = f; }

void VRSprite::setFontColor(Color4f c) { fontColor = c; }

string VRSprite::getLabel() { return label; }
Vec2f VRSprite::getSize() { return Vec2f(width, height); }

void VRSprite::setSize(float w, float h) {
    width = w;
    height = h;
    setMesh(makePlaneGeo(width, height, 1, 1));
}

OSG_END_NAMESPACE;
