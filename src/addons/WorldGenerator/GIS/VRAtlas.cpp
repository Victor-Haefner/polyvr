#include "VRAtlas.h"

#include "core/utils/toString.h"
#include "core/scene/VRScene.h"
#include "core/utils/system/VRSystem.h"
#include "core/math/pose.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRCamera.h"

#ifndef WITHOUT_GDAL
#include "core/scene/import/GIS/VRGDAL.h"
#endif
#include "core/objects/material/VRTexture.h"

#include <iostream>
#include <fstream>
#include <cmath>

using namespace OSG;

template<> string typeName(const VRAtlas& p) { return "Atlas"; }

VRAtlas::VRAtlas() {}
VRAtlas::~VRAtlas() {}

VRAtlasPtr VRAtlas::create() { return VRAtlasPtr( new VRAtlas() ); }
//VRAtlasPtr VRAtlas::ptr() { return static_pointer_cast<VRAtlasPtr>(shared_from_this()); }

void VRAtlas::test() { cout << "VRAtlas::test" << endl; }

VRTerrainPtr VRAtlas::generateTerrain(string id, float edgeLength){
    string pathOrtho = "data/test64x64.jpg";
    string pathHeight = "data/testW64x64.jpg";
    string name = "testTer" + id;
    //VRTexturePtr heightIMG = loadGeoRasterData(pathHeight, false);
    VRTexturePtr heightIMG = VRTexture::create();
    heightIMG->read(pathHeight);
    Color4f mixColor = Color4f(1,1,1,1);
    float mixAmount = 0;
    Vec2d terrainSize = Vec2d(edgeLength,edgeLength);
    //float scale = 64.0/edgeLength;
    float scale = 1;

    auto terrain = VRTerrain::create(name);
    terrain->setParameters (terrainSize, scale, 1);
    terrain->paintHeights( pathOrtho, mixColor, mixAmount );
    terrain->setMap( heightIMG, 3 );
    //terrain->setWorld( ptr() );
    //terrain->setLODFactor(fac);
    //terrain->setLit(isLit);

    return terrain;
}

VRGeometryPtr VRAtlas::generatePatch(string id) {
    VRGeoData gdata = VRGeoData();
    string name = "testGeo" + id;
    Vec3d origin = Vec3d(0,0,0);
    Vec3d pos0 = origin;
    float l = 1;
    Vec3d pos1 = origin + Vec3d(l,0,0);
    Vec3d pos2 = origin + Vec3d(0,0,l);
    Vec3d pos3 = origin + Vec3d(l,0,l);

    gdata.pushVert(pos0);
    gdata.pushVert(pos1);
    gdata.pushVert(pos2);
    gdata.pushVert(pos3);

    gdata.pushTri(0,1,2);
    gdata.pushTri(1,3,2);
    VRGeometryPtr geo = VRGeometry::create(name);
    gdata.apply(geo);

    return geo;
}

void VRAtlas::update() {
    auto camDef = VRScene::getCurrent()->getActiveCamera();
    auto defCamPos = camDef->getWorldPosition();//camDef->getFrom();

    //cout << defCamPos << endl;
}

VRTransformPtr VRAtlas::setup() {
    cout << "VRAtlas::setup" << endl;

    updatePtr = VRUpdateCb::create("atlas update", bind(&VRAtlas::update, this));
    VRScene::getCurrent()->addUpdateFkt(updatePtr);

    atlas = VRTransform::create("testTransform");
    float size = 64.0;
    float nSize;
    int fac = 1;
    string id = "";

    auto geo = generatePatch("000");
    atlas->addChild(geo);

    auto check = [&](int a, int b){
        if (a < 2) return true;
        if (b < 2) return true;
        if (a > 5) return true;
        if (b > 5) return true;
        return false;
    };

    auto addRing = [&](){
        //secondRing
        fac = fac * 2;
        nSize = size*fac;
        for (int i = 0; i < 8; i++){
            for (int j = 0; j < 8; j++){
                if (check(i,j)){
                    id = "_R" + toString(fac) + "_" + toString(i) + toString(j);
                    auto ter = generateTerrain( id , nSize );
                    atlas->addChild(ter);
                    ter->setTransform(Vec3d((i-4)*nSize+nSize*0.5,0,(j-4)*nSize+nSize*0.5));
                }
            }
        }
        cout << 8*nSize << "m" << endl;
    };

    //innerQuad
    fac = 1;
    for (int i = 0; i < 8; i++){
        for (int j = 0; j < 8; j++){
            id = "_inQ" + toString(i) + toString(j);
            auto ter = generateTerrain( id , size );
            atlas->addChild(ter);
            ter->setTransform(Vec3d((i-4)*size+size*0.5,0,(j-4)*size+size*0.5));
        }
    }
    /*fac = fac * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2;
    addRing();*/

    for (int r = 0; r < 8; r++){
        addRing();
    }


    //geo.setTransform(Vec3d(0,0,0));

    //Vec3d nor;
    //gdata.pushNorm(nor);

    return atlas;
}
