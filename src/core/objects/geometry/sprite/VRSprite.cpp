#include "VRSprite.h"

#include "VRSpriteResizeTool.h"
#include "core/tools/VRText.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/OSGGeometry.h"
#ifndef WITHOUT_BULLET
#include "core/objects/geometry/VRPhysics.h"
#endif
#include "core/objects/geometry/VRGeoData.h"
#include "core/tools/VRAnnotationEngine.h"
#include "core/tools/VRGeoPrimitive.h"
#include "core/setup/VRSetup.h"
#ifndef WITHOUT_CEF
#include "addons/CEF/CEF.h"
#endif
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGGeoProperties.h>
#include <sstream>


using namespace OSG;

template<> string typeName(const VRSprite& o) { return "Sprite"; }


VRSprite::VRSprite (string name, bool alpha, float w, float h) : VRGeometry(name) {
    width = w;
    height = h;
    type = "Sprite";
}

VRSprite::~VRSprite() {}

VRSpritePtr VRSprite::create(string name, bool alpha, float w, float h) {
    auto s = shared_ptr<VRSprite>(new VRSprite(name, alpha, w, h) );
    s->updateGeo();
    return s;
}

VRSpritePtr VRSprite::ptr() { return static_pointer_cast<VRSprite>( shared_from_this() ); }

void VRSprite::updateGeo() { // TODO: plane primitive would be better, but backwards compatibility??
    VRGeoData data;
    float w2 = width*0.5;
    float h2 = height*0.5;
    data.pushVert(Pnt3d(-w2,h2,0), Vec3d(0,0,1), Vec2d(0,1));
    data.pushVert(Pnt3d(-w2,-h2,0), Vec3d(0,0,1), Vec2d(0,0));
    data.pushVert(Pnt3d(w2,-h2,0), Vec3d(0,0,1), Vec2d(1,0));
    data.pushVert(Pnt3d(w2,h2,0), Vec3d(0,0,1), Vec2d(1,1));
#ifndef __EMSCRIPTEN__
    data.pushQuad();
#else
    data.pushTri(-4,-3,-2);
    data.pushTri(-4,-2,-1);
#endif
    data.apply(ptr());
}

VRTexturePtr VRSprite::setText(string l, float res, Color4f c1, Color4f c2, int ol, Color4f oc, string font) {
    label = l;
    auto m = VRMaterial::create(getName()+"label");

    auto tex = VRText::get()->create(l, font, res, 3, c1, c2, ol, oc);
    //float tW = tex->getSize()[0];
    //float lW = VRText::get()->layoutWidth;
    //texPadding = padding / tW;
    //charTexSize = lW/tW / cN;

    m->setTexture(tex);
    setMaterial(m);
    return tex;
}

void VRSprite::webOpen(string path, int res, float ratio) {
#ifndef WITHOUT_CEF
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
#endif
}

void VRSprite::setTexture(string path) {
    auto m = VRMaterial::create(path);
    m->setTexture(path);
    setMaterial(m);
}

string VRSprite::getLabel() { return label; }
Vec2d VRSprite::getSize() { return Vec2d(width, height); }

void VRSprite::setSize(float w, float h) {
    width = w;
    height = h;
    updateGeo();
}

void VRSprite::showResizeTool(bool b, float size, bool doAnnotations) {
    if (!b && resizeTool) resizeTool->select(false);
    if (b) {
        if (!resizeTool) {
            setPrimitive("Plane "+toString(width)+" "+toString(height)+" 1 1");
            resizeTool = VRGeoPrimitive::create();
            resizeTool->setGeometry(ptr());
            addChild(resizeTool);
        }
        resizeTool->select(true);
        resizeTool->setHandleSize(size);
        resizeTool->getLabels()->setVisible(doAnnotations);
    }
}

void VRSprite::convertToCloth() {
#ifndef WITHOUT_BULLET
    getPhysics()->setDynamic(true);
    getPhysics()->setShape("Cloth");
    getPhysics()->setSoft(true);
    getPhysics()->setPhysicalized(true);
#endif
}

