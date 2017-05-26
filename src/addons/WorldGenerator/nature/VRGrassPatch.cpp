#include "VRGrassPatch.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/math/polygon.h"

#include "core/objects/VRCamera.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/geometry/VRSprite.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/tools/VRTextureRenderer.h"
#include "core/scene/VRScene.h"

const double pi = 2*acos(0.0);

using namespace OSG;

VRGrassPatch::VRGrassPatch() : VRTransform("grass_patch") {}

VRGrassPatch::~VRGrassPatch() {}
VRGrassPatchPtr VRGrassPatch::create() { return VRGrassPatchPtr( new VRGrassPatch() ); }

void VRGrassPatch::setArea(PolygonPtr p) {
    area = p;
    VRGeoData patch;
    //createPatch(patch, area);
    //addChild( patch.asGeometry("grassPatch") );
    createSpriteLOD(patch, 0);
}

void VRGrassPatch::addGrassBlade(VRGeoData& data, Vec3f pos, float a, float dh, int lvl, Vec3f c) { // Todo: replace by triangle strip
    float h = bladeHeight*dh;
    float w = bladeHeight*0.05; // 0.01 half blade width
    float b = bladeHeight*0.5; // blade bending

    float ca = cos(a);
    float sa = sin(a);

    auto rot = [&](const Vec3f& v) {
        float X = v[0]*ca - v[2]*sa;
        float Y = v[0]*sa + v[2]*ca;
        return Vec3f(X, v[1], Y);
    };

    float h1 = h/3;
    float h2 = h*2.0/3;
    float b1 = b/6;
    float b2 = b/2;

    Vec3f p1 = rot(Vec3f( w,0, 0));
    Vec3f p2 = rot(Vec3f(-w,0, 0));
    Vec3f p3 = rot(Vec3f( w,h1,b1));
    Vec3f p4 = rot(Vec3f(-w,h1,b1));
    Vec3f p5 = rot(Vec3f( w,h2,b2));
    Vec3f p6 = rot(Vec3f(-w,h2,b2));
    Vec3f p7 = rot(Vec3f( 0,h, b));

    Vec3f n0 = rot(Vec3f(0,0,-1));
    Vec3f n1 = rot(Vec3f(0,b1,-h1));
    Vec3f n2 = rot(Vec3f(0,b2,-h2));
    n1.normalize();
    n2.normalize();

    int N = data.size();

    data.pushVert(pos + p1, n0, c);
    data.pushVert(pos + p2, n0, c);
    if (lvl < 2) data.pushVert(pos + p3, n1, c);
    if (lvl < 2) data.pushVert(pos + p4, n1, c);
    if (lvl < 1) data.pushVert(pos + p5, n2, c);
    if (lvl < 1) data.pushVert(pos + p6, n2, c);
    data.pushVert(pos + p7, n0, c);

    if (lvl == 0) {
        data.pushQuad(N,N+1,N+3,N+2);
        data.pushQuad(N+2,N+3,N+5,N+4);
        data.pushTri(N+4,N+5,N+6);
    }

    if (lvl == 1) {
        data.pushQuad(N,N+1,N+3,N+2);
        data.pushTri(N+2,N+3,N+4);
    }

    if (lvl >= 2) data.pushTri(N,N+1,N+2);
}

void VRGrassPatch::createPatch(VRGeoData& data, PolygonPtr area, int lvl, int density) {
    for (auto pos : getRandomPoints(area, density)) {
        float a = 2*pi*float(rand())/RAND_MAX;
        float dh = 0.5 + 0.5*float(rand())/RAND_MAX;
        addGrassBlade(data, pos, a, dh, lvl, Vec3f(0.2,0.8,0.0));
    }
}

vector<Vec3f> VRGrassPatch::getRandomPoints(PolygonPtr area, int density) {
    vector<Vec3f> points;
    int N = density*area->computeArea();
    for (int i=0; i<N; i++) points.push_back( area->getRandomPoint() );
    return points;
}

void VRGrassPatch::createSpriteLOD(VRGeoData& data, int lvl) {
    float W = 0.2;
    auto area2 = Polygon::create();
    area2->addPoint(Vec2f(-W, -W));
    area2->addPoint(Vec2f( W, -W));
    area2->addPoint(Vec2f( W,  W));
    area2->addPoint(Vec2f(-W,  W));
    VRGeoData data2;
    createPatch(data2, area2, lvl, 1000);
    auto grass = data2.asGeometry("grass");

    auto trend = VRTextureRenderer::create("grassRenderer");
    trend->setPersistency(0);
    auto cam = VRCamera::create("grassCam", false);
    auto light = VRLight::create("grassLight");
    auto lightBeacon = VRLightBeacon::create("grassLightBeacon");
    trend->addChild(light);
    light->addChild(cam);
    cam->addChild(lightBeacon);
    cam->setFov(0.33);
    light->setBeacon(lightBeacon);
    light->addChild(grass);
	lightBeacon->setFrom(Vec3f(1,1,1));
	trend->setup(cam, 512, 512, true);

    cam->setPose(Vec3f(0,2,0), Vec3f(0,-1,0), Vec3f(0,0,1)); // top
    cam->update();
    auto texTop = trend->renderOnce();
    auto matTop = VRMaterial::create("grassTop");
    matTop->setTexture(texTop);
    matTop->enableTransparency();
    matTop->setLit(false);

    cam->setPose(Vec3f(0,0,2), Vec3f(0,0,-1), Vec3f(0,1,0)); // side
    cam->update();
    auto texSide = trend->renderOnce();
    auto matSide = VRMaterial::create("grassTop");
    matSide->setTexture(texSide);
    matSide->enableTransparency();
    matSide->setLit(false);

    for (auto p : getRandomPoints(area, 10)) {
        auto spriteTop = VRSprite::create("grassTop");
        spriteTop->setSize(W*3, W*3);
        spriteTop->setMaterial(matTop);
        spriteTop->setPose(p, Vec3f(0,1,0), Vec3f(0,0,1));
        //data.append(spriteTop);
        addChild(spriteTop);
    }

    for (auto p : getRandomPoints(area, 10)) {
        auto spriteSide = VRSprite::create("grassSide");
        spriteSide->setSize(W*3, 0.4);
        spriteSide->setMaterial(matSide);
        spriteSide->setPose(p/*+Vec3f(0,0.3,0)*/, Vec3f(0,0,-1), Vec3f(0,1,0));
        //data.append(spriteSide);
        addChild(spriteSide);
    }

    for (auto p : getRandomPoints(area, 10)) {
        auto spriteFront = VRSprite::create("grassFront");
        spriteFront->setSize(W*3, 0.4);
        spriteFront->setMaterial(matSide);
        spriteFront->setPose(p, Vec3f(-1,0,0), Vec3f(0,1,0));
        //data.append(spriteFront);
        addChild(spriteFront);
    }
}

void VRGrassPatch::createLod(VRGeoData& geo, int lvl, Vec3f offset, int ID) {
    Matrix Offset;
    Offset.setTranslate(offset);

    Vec2f id = Vec2f(ID, 1); // the 1 is a flag to identify the ID as such!

    if (lods.count(lvl)) {
        geo.append(lods[lvl], Offset);
        return;
    }

    VRGeoData patch;
    createPatch(patch, area, lvl);
    //createSpriteLOD(Hull, lvl);

    lods[lvl] = patch.asGeometry("grassLod");
    geo.append(patch, Offset);
}


/**

Idea:

- terrain has texture
- render grass as second pass, using terrain info
- reuse road texture stuff!

*/







