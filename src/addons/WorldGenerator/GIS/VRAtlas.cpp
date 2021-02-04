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
#include <list>
#include <fstream>
#include <cmath>

using namespace OSG;

template<> string typeName(const VRAtlas& p) { return "Atlas"; }

VRAtlas::VRAtlas() {}
VRAtlas::~VRAtlas() {}

VRAtlasPtr VRAtlas::create() { return VRAtlasPtr( new VRAtlas() ); }
//VRAtlasPtr VRAtlas::ptr() { return static_pointer_cast<VRAtlasPtr>(shared_from_this()); }

VRAtlas::Patch::Patch(string sid, int lvl, VRTerrainPtr ter) : id(sid), LODlvl(lvl), terrain(ter) {}
VRAtlas::Patch::Patch() {}
VRAtlas::Patch::~Patch() {}

VRAtlas::Level::Level(int t, int lvl) : type(t), LODlvl(lvl) {}
VRAtlas::Level::Level() {}
VRAtlas::Level::~Level() {}

void VRAtlas::test() { cout << "VRAtlas::test" << endl; }

VRTerrainPtr VRAtlas::generateTerrain(string id, int lvl , float edgeLength){
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
    float scale = 1*float(pow(2,lvl));

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
    int tmp = layout.currentLODlvl;
    Vec3d vUp = Vec3d(0,1,0);
    Vec3d vEast = Vec3d(1,0,0);
    Vec3d vNorth = Vec3d(0,0,-1);
    Vec3d camToOrigin = defCamPos - layout.innerQuad.currentOrigin;
    float height = camToOrigin.dot(vUp);

    //float dis = (defCamPos - layout.currentOrigin).length();

    if (height > 500.0 * pow(2.0, float(tmp))) upSize();
    if (height < 500.0 * ( pow(2.0, float(tmp) -1) ) && tmp > 0) downSize();


    auto checkShift = [&](Level& lev){
        bool shifted = false;
        Vec3d camToOrigin = defCamPos - lev.currentOrigin;
        float boundary = 2*size * pow(2.0, float(lev.LODlvl));
        float east = camToOrigin.dot(vEast);
        float north = camToOrigin.dot(vNorth);
        if (east > boundary) shiftLayoutEast(lev);
        if (east <-boundary) shiftLayoutWest(lev);
        if (north > boundary) shiftLayoutNorth(lev);
        if (north <-boundary) shiftLayoutSouth(lev);
        if (east > boundary || east <-boundary || north > boundary || north <-boundary) shifted = true;
        return shifted;
    };

    if ( checkShift(layout.innerQuad) ) {
        cout << "shifted inner Quad" << endl;
    }
    //cout << tmp << "---"  << dis << "---"  << 500.0 * pow(2.0, float(tmp)) << "---"  << 500.0 * ( pow(2.0, float(tmp)) -1) << "---" << endl;

    if (layout.toDestroy.size() > 0){
        for (auto &each : layout.toDestroy){
            each.terrain->destroy();
            patchcount--;
        }
        layout.toDestroy.clear();
    }
    //cout << defCamPos << endl;
}

void VRAtlas::shiftLayoutEast(Level& lev) {
    cout << " VRAtlas::need to shift EAST" << endl;
    int lvl = lev.LODlvl;
    Vec3d east = Vec3d(1,0,0);
    lev.currentOrigin = lev.currentOrigin + size * pow(2.0, float(lvl))*east;
    int fac = pow ( 2, lvl );
    float nSize = size*fac;
    int i = 0;
    int j = 0;
    auto shiftOnce = [&](){
        for (auto& eachRow : lev.patches) {
            Patch westPatch = eachRow.front();
            auto ter = generateTerrain(westPatch.id, lvl , size);
            //ter->setTransform(Vec3d((i-2)*nSize+nSize*0.5,0,(j-2)*nSize+nSize*0.5));
            westPatch.terrain = ter;
            eachRow.push_back(westPatch);
        }
    };


    shiftOnce;
    shiftOnce;
    if (lev.type == INNERQUAD) {
        if (lvl == layout.currentMaxLODlvl) return;
        //shift inner ring inside
    }
    else {
        //shift inner of next ring
        if (lvl == layout.currentMaxLODlvl) return;
    }
}

void VRAtlas::shiftLayoutWest(Level& lev) {
    cout << " VRAtlas::need to shift WEST" << endl;
    if (lev.type == INNERQUAD) {

    }
    else {

    }
}

void VRAtlas::shiftLayoutNorth(Level& lev) {
    cout << " VRAtlas::need to shift NORTH" << endl;
    if (lev.type == INNERQUAD) {

    }
    else {

    }
}

void VRAtlas::shiftLayoutSouth(Level& lev) {
    cout << " VRAtlas::need to shift SOUTH" << endl;
    if (lev.type == INNERQUAD) {

    }
    else {

    }
}

void VRAtlas::downSize() {
    layout.currentLODlvl--;
    layout.currentMaxLODlvl--;
    //destroy innerGrid
    for (auto eachRow : layout.innerQuad.patches) {
        for (auto each : eachRow) layout.toDestroy.push_back(each);
    }
    for (auto eachRow : layout.outerRing.patches) {
        for (auto each : eachRow) layout.toDestroy.push_back(each);
    }
    layout.levels.pop_back(); //outerRing;
    layout.outerRing = layout.levels.back();
    //make new innerGrid and new innerRing
    addInnerQuad(layout.currentLODlvl);
    addInnerRing(layout.currentLODlvl);
    //cout << "VRAtlas::downSize to lvl: " << layout.currentLODlvl << " -- "<< patchcount << endl;
}

void VRAtlas::upSize() {
    layout.currentLODlvl++;
    layout.currentMaxLODlvl++;
    //destroy innerGrid
    for (auto eachRow : layout.innerQuad.patches) {
        for (auto each : eachRow) layout.toDestroy.push_back(each);
    }
    for (auto eachRow : layout.innerRing.patches) {
        for (auto each : eachRow) layout.toDestroy.push_back(each);
    }
    layout.levels.pop_front(); //innerRing
    layout.innerRing = layout.levels.front();
    //make new innerGrid and outerRing
    addInnerQuad(layout.currentLODlvl);
    addOuterRing(layout.currentMaxLODlvl);
    //cout << "VRAtlas::upSize to lvl: " << layout.currentLODlvl << " iRs: "<< layout.innerRing.size() << " -- "<< patchcount <<  endl;
}

void VRAtlas::addInnerQuad(int lvl) {
    auto check = [&](int a, int b){
        if (a > 2) return true;
        if (b > 2) return true;
        if (a < 5) return true;
        if (b < 5) return true;
        return false;
    };
    Level lev = Level(INNERQUAD,lvl);
    int fac = pow ( 2, lvl );
    float nSize = size*fac;
    for (int i = 0; i < 8; i++){
        vector<Patch> row;
        for (int j = 0; j < 8; j++){
            if (check(i,j)){
                string id = "LOD_" + toString(lvl) + "_" + toString(fac) + "_" + toString(i) + toString(j);
                auto ter = generateTerrain( id , lvl, nSize );
                atlas->addChild(ter);
                ter->setTransform(Vec3d((i-4)*nSize+nSize*0.5,0,(j-4)*nSize+nSize*0.5));
                Patch p = Patch(id, lvl, ter);
                row.push_back(p);
                patchcount++;
            }
        }
        lev.patches.push_back(row);
    }
    layout.innerQuad = lev;
    //cout << 8*nSize << "m InnerQuad" << endl;
}

void VRAtlas::addInnerRing(int lvl) {
    auto check = [&](int a, int b){
        if (a < 2) return true;
        if (b < 2) return true;
        if (a > 5) return true;
        if (b > 5) return true;
        return false;
    };
    Level lev = Level(INNERRING,lvl);
    int fac = pow ( 2, lvl );
    float nSize = size*fac;
    for (int i = 0; i < 8; i++){
        vector<Patch> row;
        for (int j = 0; j < 8; j++){
            if (check(i,j)){
                string id = "LOD_" + toString(lvl) + "_" + toString(fac) + "_" + toString(i) + toString(j);
                auto ter = generateTerrain( id , lvl, nSize );
                atlas->addChild(ter);
                ter->setTransform(Vec3d((i-4)*nSize+nSize*0.5,0,(j-4)*nSize+nSize*0.5));
                Patch p = Patch(id, lvl, ter);
                row.push_back(p);
                patchcount++;
            }
        }
        lev.patches.push_back(row);
    }
    layout.innerRing = lev;
    layout.levels.push_front(lev);
    //cout << 8*nSize << "m InnerRing " << endl;
}

void VRAtlas::addOuterRing(int lvl) {
    auto check = [&](int a, int b){
        if (a < 2) return true;
        if (b < 2) return true;
        if (a > 5) return true;
        if (b > 5) return true;
        return false;
    };

    Level lev = Level(OUTERRING,lvl);
    int fac = pow ( 2, lvl );
    float nSize = size*fac;
    for (int i = 0; i < 8; i++){
        vector<Patch> row;
        for (int j = 0; j < 8; j++){
            if (check(i,j)){
                string id = "LOD_" + toString(lvl) + "_" + toString(fac) + "_" + toString(i) + toString(j);
                auto ter = generateTerrain( id , lvl, nSize );
                atlas->addChild(ter);
                ter->setTransform(Vec3d((i-4)*nSize+nSize*0.5,0,(j-4)*nSize+nSize*0.5));
                Patch p = Patch(id, lvl, ter);
                row.push_back(p);
                patchcount++;
            }
        }
        lev.patches.push_back(row);
    }
    layout.levels.push_back(lev);
    layout.outerRing = lev;
    //cout << 8*nSize << "m OuterRing" << endl;
}

VRTransformPtr VRAtlas::setup() {
    cout << "VRAtlas::setup" << endl;

    updatePtr = VRUpdateCb::create("atlas update", bind(&VRAtlas::update, this));
    VRScene::getCurrent()->addUpdateFkt(updatePtr);

    atlas = VRTransform::create("testTransform");

    auto geo = generatePatch("000");
    atlas->addChild(geo);

    //LODMax = 5;

    addInnerQuad(0);
    addInnerRing(0);

    for (int i = 0; i < LODMax; i++){
        layout.currentMaxLODlvl++;
        addOuterRing(layout.currentMaxLODlvl);
    }
    //geo.setTransform(Vec3d(0,0,0));

    //Vec3d nor;
    //gdata.pushNorm(nor);

    return atlas;
}
