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

VRSprite::~VRSprite() {
    ;
}

void VRSprite::setLabel (string l, float res) {
    if (l == label) return;
    label = l;
    auto labelMat = VRText::get()->getTexture(l, font, 20*res, fontColor, Color4f(0,0,0,0) );
    labelMat->setSortKey(1);
    setMaterial(labelMat);
}

void VRSprite::webOpen(string path, int res, float ratio){
    VRMaterial* mat = VRMaterial::get(getName()+"web");
    setMaterial(mat);
    mat->setLit(false);
    CEF* w = new CEF();

    VRDevice* mouse = VRSetupManager::getCurrent()->getDevice("mouse");
    VRDevice* keyboard = VRSetupManager::getCurrent()->getDevice("keyboard");

    w->setMaterial(mat);
    w->open(path);
    w->addMouse(mouse, this, 0, 2, 3, 4);
    w->addKeyboard(keyboard);
    w->setResolution(res);
    w->setAspectRatio(ratio);
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
