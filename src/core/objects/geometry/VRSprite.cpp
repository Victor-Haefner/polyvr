#include "VRSprite.h"

#include "core/tools/VRText.h"
#include "core/objects/material/VRMaterial.h"
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

void VRSprite::setLabel (string l, float w, float h) {
    if (l == label) return;
    w = l.size()*20;
    label = l;
    labelMat = VRText::get()->getTexture(l, font, w, h, fontColor, Color4f(0,0,0,0) );
    labelMat->setSortKey(1);
    getMesh()->setMaterial(labelMat);
}

void VRSprite::setTexture(string path){
    getMaterial()->setTexture(path);
    setMaterial(getMaterial());
}

void VRSprite::setFont(string f) { font = f; }

void VRSprite::setFontColor(Color4f c) { fontColor = c; }

string VRSprite::getLabel() { return label; }

void VRSprite::setSize(float w, float h) {
    width = w;
    height = h;
    setMesh(makePlaneGeo(width, height, 1, 1));
    getMesh()->setMaterial(labelMat);
}

OSG_END_NAMESPACE;
