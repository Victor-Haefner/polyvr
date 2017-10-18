#include "VRGrassPatch.h"

#include "VRPlantMaterial.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/math/polygon.h"
#include "core/math/triangulator.h"

#include "core/objects/VRCamera.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/VRLod.h"


#include "core/objects/geometry/VRSprite.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/tools/VRTextureRenderer.h"
#include "core/utils/toString.h"
#include "core/scene/VRScene.h"


const double pi = 2*acos(0.0);
using namespace OSG;

VRGrassPatch::VRGrassPatch() : VRTransform("grass_patch") {}

VRGrassPatch::~VRGrassPatch() {}
VRGrassPatchPtr VRGrassPatch::create() { return VRGrassPatchPtr( new VRGrassPatch() ); }

void VRGrassPatch::initLOD() {
    auto createGrass = [&](VRPolygonPtr a, int i, float blades, float patches, VRLodPtr lod) {
        VRGeoData geo1, geo2;
        createPatch(geo1, a, i, 800*blades);
        createSpriteLOD(geo2, a, i, 80*patches);

        if (geo1.size() > 0) {
            auto grass = geo1.asGeometry("grassPatchBlades");
            grass->setMaterial(matGrass);
            lod->getChild(i)->addChild(grass);
        }

        if (geo2.size() > 0) {
            auto grass = geo2.asGeometry("grassPatchLod");
            grass->setMaterial(matGrassSide);
            lod->getChild(i)->addChild(grass);
        }
    };

    for (auto a : chunks) {
        lod = VRLod::create("grass_lod");
        lod->setPersistency(0);
        lod->setCenter(a->getBoundingBox().center());
        addChild(lod);
        for (int i=0; i<3; i++) {
            auto lodI = VRObject::create("grass_lod"+toString(i));
            lodI->setPersistency(0);
            lod->addChild(lodI);
        }

        createGrass(a, 0, 1, 0, lod);
        lod->addDistance(2);
        createGrass(a, 1, 0.5, 0.5, lod);
        lod->addDistance(4);
        createGrass(a, 2, 0, 1, lod);
    }
}

void VRGrassPatch::setArea(VRPolygonPtr p) {
    area = p;
    chunks = p->gridSplit(1.0);
    setupGrassMaterials();
    setupGrassStage();
    initLOD();
}

void VRGrassPatch::addGrassBlade(VRGeoData& data, Vec3d pos, float a, float dh, int lvl, Color3f c) { // Todo: replace by triangle strip
    float h = bladeHeight*dh;
    float w = bladeHeight*0.05; // 0.01 half blade width
    float b = bladeHeight*0.5; // blade bending
    w *= (lvl+1);

    float ca = cos(a);
    float sa = sin(a);

    auto rot = [&](const Vec3d& v) {
        float X = v[0]*ca - v[2]*sa;
        float Y = v[0]*sa + v[2]*ca;
        return Vec3d(X, v[1], Y);
    };

    float h1 = h/3;
    float h2 = h1*2;
    float b1 = b/6;
    float b2 = b/2;

    Vec3d p1 = rot(Vec3d( w,0, 0));
    Vec3d p2 = rot(Vec3d(-w,0, 0));
    Vec3d p3 = rot(Vec3d( w,h1,b1));
    Vec3d p4 = rot(Vec3d(-w,h1,b1));
    Vec3d p5 = rot(Vec3d( w,h2,b2));
    Vec3d p6 = rot(Vec3d(-w,h2,b2));
    Vec3d p7 = rot(Vec3d( 0,h, b));

    Vec3d n0 = rot(Vec3d(0,0,-1));
    Vec3d n1 = rot(Vec3d(0,b1,-h1));
    Vec3d n2 = rot(Vec3d(0,b2,-h2));
    Vec3d n3 = rot(Vec3d(0,b,-h));
    n1.normalize();
    n2.normalize();
    n3.normalize();

    int N = data.size();

    data.pushVert(pos + p1, n0, c, Vec2d(0,0));
    data.pushVert(pos + p2, n0, c, Vec2d(1,0));
    if (lvl <= 1) data.pushVert(pos + p3, n1, c, Vec2d(0,0.333));
    if (lvl <= 1) data.pushVert(pos + p4, n1, c, Vec2d(1,0.333));
    if (lvl == 0) data.pushVert(pos + p5, n2, c, Vec2d(0,0.666));
    if (lvl == 0) data.pushVert(pos + p6, n2, c, Vec2d(1,0.666));
    data.pushVert(pos + p7, n3, c, Vec2d(0.5,1));

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

void VRGrassPatch::createPatch(VRGeoData& data, VRPolygonPtr area, int lvl, float density) {
    for (auto pos : area->getRandomPoints(density*1.0/(lvl+1), 0)) {
        float a = 2*pi*float(rand())/RAND_MAX;
        float dh = 0.5 + 0.5*float(rand())/RAND_MAX;
        addGrassBlade(data, pos, a, dh, lvl, Color3f(0.2,0.8,0.0));
    }
}

VRTextureRendererPtr VRGrassPatch::texRenderer = 0;
VRPlantMaterialPtr VRGrassPatch::matGrass = 0;
VRPlantMaterialPtr VRGrassPatch::matGrassUnlit = 0;
VRPlantMaterialPtr VRGrassPatch::matGrassSide = 0;

void VRGrassPatch::setupGrassMaterials() {
    if (matGrass) return;
    matGrass = VRPlantMaterial::create();
    matGrassUnlit = VRPlantMaterial::create();
    matGrassSide = VRPlantMaterial::create();

    VRTextureGenerator tg;
    tg.setSize( Vec3i(16,64,1), true );
    tg.drawFill( Color4f(0.1,0.3,0.1,1.0) );
    tg.drawLine( Vec3d(0.75,0,0), Vec3d(0.75,1,0), Color4f(0.2,0.5,0.1,1.0), 0.5 );

    auto tex = tg.compose(0);
    matGrass->setTexture(tex);
    matGrassUnlit->setTexture(tex);

    matGrassUnlit->setLit(false);
    matGrassUnlit->composeShader();

    matGrassSide->enableTransparency();
    //matGrassSide->setLit(false);
    //matGrassSide->composeShader();
}

void VRGrassPatch::setupGrassStage() {
    if (texRenderer) return;

    float W = 0.1;
    auto area2 = VRPolygon::create();
    area2->addPoint(Vec2d(-W, -W));
    area2->addPoint(Vec2d( W, -W));
    area2->addPoint(Vec2d( W,  W));
    area2->addPoint(Vec2d(-W,  W));
    VRGeoData data2;
    createPatch(data2, area2, 0, 1000);
    auto grass = data2.asGeometry("grass");
    //grass->setMaterial(matGrass);
    grass->setMaterial(matGrassUnlit);

    texRenderer = VRTextureRenderer::create("grassRenderer");
    texRenderer->setBackground(Color3f(0,0.8,0));
    texRenderer->setPersistency(0);
    auto cam = VRCamera::create("grassCam", false);
    auto light = VRLight::create("grassLight");
    auto lightBeacon = VRLightBeacon::create("grassLightBeacon");
    texRenderer->addChild(light);
    light->addChild(cam);
    cam->addChild(lightBeacon);
    cam->setFov(0.33); //0.33
    cam->setAspect(0.2);
    light->setBeacon(lightBeacon);
    light->addChild(grass);
    light->setType("directional");
	lightBeacon->setPose(Vec3d(), Vec3d(0.5,-1,-1), Vec3d(0,0,1));
	texRenderer->setup(cam, 512, 512, true);

    cam->setPose(Vec3d(0,0,-2), Vec3d(0,0,1), Vec3d(0,1,0)); // side
    cam->updateChange();
    auto texSide = texRenderer->renderOnce();
    matGrassSide->setTexture(texSide);
}

VRMaterialPtr VRGrassPatch::getGrassMaterial() { setupGrassMaterials(); return matGrass; }
VRMaterialPtr VRGrassPatch::getGrassSideMaterial() { return matGrassSide; }

void VRGrassPatch::createSpriteLOD(VRGeoData& data, VRPolygonPtr area, int lvl, float density) {
    for ( auto p : area->getRandomPoints(density,0) ) {
        float r = float(random())/RAND_MAX;
        data.pushQuad(p, Vec3d(1-r,0.1,-r), Vec3d(0,1,0), Vec2d(0.2, 0.65), true);
    }
}

void VRGrassPatch::createLod(VRGeoData& geo, int lvl, Vec3d offset, int ID) {
    Matrix4d Offset;
    Offset.setTranslate(offset);

    if (lods.count(lvl)) {
        geo.append(lods[lvl], Offset);
        return;
    }

    VRGeoData patch;
    createSpriteLOD(patch, area, lvl, 100.0/lvl);
    if (!patch.size()) return;
    //for (auto c : chunks) { createSpriteLOD(patch, c, lvl); }
    auto sprites = patch.asGeometry("grassPatch");

    lods[lvl] = sprites;
    geo.append(patch, Offset);
}


/**

Idea:

- terrain has texture
- render grass as second pass, using terrain info
- reuse road texture stuff!

*/







