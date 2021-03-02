#include "VRAtlas.h"

#include "core/utils/toString.h"
#include "core/scene/VRScene.h"
#include "core/utils/system/VRSystem.h"
#include "core/math/pose.h"
#include "core/objects/VRCamera.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"

#ifndef WITHOUT_GDAL
#include "core/scene/import/GIS/VRGDAL.h"
#endif

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

VRAtlas::Boundary::Boundary(double minEast, double maxEast, double minNorth, double maxNorth) : minEast(minEast), maxEast(maxEast), minNorth(minNorth), maxNorth(maxNorth) {}
VRAtlas::Boundary::Boundary() {}
VRAtlas::Boundary::~Boundary() {}

VRAtlas::Patch::Patch(string sid, int lvl, VRTerrainPtr ter) : id(sid), LODlvl(lvl), terrain(ter) {}
VRAtlas::Patch::Patch() {}
VRAtlas::Patch::~Patch() {}

VRAtlas::Level::Level(int t, int lvl) : type(t), LODlvl(lvl) {}
VRAtlas::Level::Level() {}
VRAtlas::Level::~Level() {}

VRAtlas::Layout::Layout() {}
VRAtlas::Layout::~Layout() {}


VRAtlas::Layout::setCoords(Patch& pat, Vec3d co3) {
    pat->coords = Vec2d(co3[0],co3[2]);
    Vec3d pos = co3 + Vec3d(nSize*0.5,0,nSize*0.5);
    pat->terrain->setTransform(pos);

    string pathOrtho = "data/test64x64.jpg";
    string pathHeight = "data/testW64x64.jpg";

    VRTexturePtr heightIMG = VRTexture::create();
    heightIMG->read(pathHeight);

    pat->terrain->paintHeights(pathOrtho);
    pat->terrain->setMap( heightIMG, 3 );
}

void VRAtlas::Layout::shiftEastIns(Level& lev, list<Level>::iterator it) {
    cout << " VRAtlas::need to shift EAST inside:" << endl;
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d east = Vec3d(1,0,0);

    if (lev.type == INNERQUAD) { return; }
    else {
        //shift inner of next ring
        if (lvl == currentMaxLODlvl) return;
    }
}

void VRAtlas::Layout::shiftEastOut(Level& lev, list<Level>::iterator it) {
    cout << " VRAtlas::need to shift EAST outside:" << endl;
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d east = Vec3d(1,0,0);
    lev.currentOrigin = lev.currentOrigin + east*2*nSize;

    auto checkIR = [&](int a, int b){
        if ( a >= 6 && a < lev.patches.size() && b >= 2 && b < lev.patches[a].size() - 2) return true;
        return false;
    };

    if (lev.type == INNERQUAD) {
        //transfer patches' ownership to inner ring
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (i < 2) {
                    Patch nPatch = lev.patches[i][j];
                    innerRing.patches[i+2].insert(innerRing.patches[i+2].begin()+2+j,nPatch);
                }
            }
        }
        //erase patches in inner quad
        for (int i = 0; i < 2; i++) {
            lev.patches.erase(lev.patches.begin());
        }
        shiftEastOut(innerRing, it);
    }

    if (lev.type == INNERRING) {
        //transfer patches' ownership to inner quad
        for (int i = 6; i < 8; i++) {
            vector<Patch> col;
            for (int j = 2; j < 6; j++) {
                if (checkIR(i,j)) col.push_back(lev.patches[i][j]);
            }
            innerQuad.patches.push_back(col);
        }
        //erase patches in inner ring
        for (int i = 6; i < 8; i++) {
            for (int j = 2; j < 6; j++) lev.patches[i].erase(lev.patches[i].begin()+2);
        }
        //move patches on inner ring
        for (int i = 0; i < 2; i++) { //transfer patches ownership to next bigger ring
            for (int j = 0; j < 8; j++){
                auto patch = lev.patches[i][j];
                //patch.terrain->setTransform(lev.currentOrigin + Vec3d((i-4)*nSize+nSize*0.5,0,(j-4+6)*nSize+nSize*0.5));
                patch.terrain->setTransform(lev.currentOrigin + Vec3d((i+6-4)*nSize+nSize*0.5,0,(j-4)*nSize+nSize*0.5));
                ///TODO: swap textures and heightmap
                //patch.terrain->paintHeights("data/test64x64Invert.jpg");
            }
        }
        for (int i = 0; i < 2; i++) {
            auto col = lev.patches.front();
            innerRing.patches.push_back(col);
            innerRing.patches.erase(innerRing.patches.begin());
        }
        auto it = levels.begin();
        it++;
        //shiftSouthIns(*it, it);
    }
}

void VRAtlas::Layout::shiftWestIns(Level& lev, list<Level>::iterator it) {
    cout << " VRAtlas::need to shift WEST inside:" << endl;
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d east = Vec3d(1,0,0);

    if (lev.type == INNERQUAD) { return; }
    else {

    }
}

void VRAtlas::Layout::shiftWestOut(Level& lev, list<Level>::iterator it) {
    cout << " VRAtlas::need to shift WEST outside:" << endl;
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d east = Vec3d(1,0,0);
    lev.currentOrigin = lev.currentOrigin - east*2*nSize;

    if (lev.type == INNERQUAD) {
        //transfer patches' ownership to inner ring
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (i >= 2) {
                    Patch nPatch = lev.patches[i][j];
                    innerRing.patches[i+2].insert(innerRing.patches[i+2].begin()+2+j,nPatch);
                }
            }
        }
        //erase patches in inner quad
        for (int i = 2; i < 4; i++) {
            lev.patches.pop_back();
        }
        shiftWestOut(innerRing, it);
    }

    if (lev.type == INNERRING) {
        //transfer patches' ownership to inner quad
        for (int i = 0; i < 2; i++) {
            vector<Patch> col;
            for (int j = 2; j < 6; j++) {
                col.push_back(lev.patches[i][j]);
            }
            innerQuad.patches.insert(innerQuad.patches.begin()+i,col);
        }
        //erase patches in inner ring
        for (int i = 0; i < 2; i++) {
            for (int j = 2; j < 6; j++) lev.patches[i].erase(lev.patches[i].begin()+2);
        }
        //move patches on inner ring
        for (int i = 7; i >= 6; i--) { //transfer patches ownership to next bigger ring
            for (int j = 0; j < 8; j++){
                auto patch = lev.patches[i][j];
                //patch.terrain->setTransform(lev.currentOrigin + Vec3d((i-4)*nSize+nSize*0.5,0,(j-4+6)*nSize+nSize*0.5));
                patch.terrain->setTransform(lev.currentOrigin + Vec3d((i-6-4)*nSize+nSize*0.5,0,(j-4)*nSize+nSize*0.5));
                ///TODO: swap textures and heightmap
                //patch.terrain->paintHeights("data/test64x64Invert.jpg");
            }
        }
        for (int i = 0; i < 2; i++) {
            auto col = lev.patches.back();
            lev.patches.insert(lev.patches.begin(),col);
            lev.patches.pop_back();
        }
        auto it = levels.begin();
        it++;
        //shiftSouthIns(*it, it);
    }
}

void VRAtlas::Layout::shiftNorthIns(Level& lev, list<Level>::iterator it) {
    cout << " VRAtlas::need to shift NORTH inside:" << endl;
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d north = Vec3d(0,0,-1);

    if (lev.type == INNERQUAD) { return; }
    else {

    }
}

void VRAtlas::Layout::shiftNorthOut(Level& lev, list<Level>::iterator it) {
    cout << " VRAtlas::need to shift NORTH outside:" << endl;
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d north = Vec3d(0,0,-1);
    lev.currentOrigin = lev.currentOrigin + north*2*nSize;

    auto checkIR = [&](int a, int b){
        if ( a >= 2 && a < lev.patches.size() - 2 && b < 2 ) return true;
        return false;
    };

    if (lev.type == INNERQUAD) {
        //transfer patches' ownership to inner ring
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (j >= 2) {
                    Patch nPatch = lev.patches[i][j];
                    innerRing.patches[i+2].insert(innerRing.patches[i+2].begin()+2,nPatch);
                }
            }
        }
        //erase patches in inner quad
        for (int i = 0; i < 4; i++) {
            for (int j = 2; j < 4; j++) lev.patches[i].erase(lev.patches[i].end());
        }
        shiftNorthOut(innerRing, it);
    }

    if (lev.type == INNERRING) {
        //transfer patches' ownership to inner quad
        for (int i = 2; i < 6; i++) {
            for (int j = 2; j >= 0; j--) {
                if (checkIR(i,j)) innerQuad.patches[i-2].insert(innerQuad.patches[i-2].begin(),lev.patches[i][j]);
            }
        }
        //erase patches in inner ring
        for (int i = 2; i < 6; i++) {
            for (int j = 0; j < 2; j++) lev.patches[i].erase(lev.patches[i].begin());
        }
        //move patches on inner ring
        for (int i = 0; i < lev.patches.size(); i++) { //transfer patches ownership to next bigger ring
            for (int j = 1; j >= 0; j--) {
                auto patch = lev.patches[i].back();
                innerRing.patches[i].insert(innerRing.patches[i].begin(),patch);
                innerRing.patches[i].pop_back();
                //patch.terrain->setTransform(lev.currentOrigin + Vec3d((i-4)*nSize+nSize*0.5,0,(j-4+6)*nSize+nSize*0.5));
                patch.terrain->setTransform(lev.currentOrigin + Vec3d((i-4)*nSize+nSize*0.5,0,(j-4)*nSize+nSize*0.5));
                ///TODO: swap textures and heightmap
                //patch.terrain->paintHeights("data/test64x64Invert.jpg");
            }
        }
        auto it = levels.begin();
        it++;
        //shiftSouthIns(*it, it);
    }

}

void VRAtlas::Layout::shiftSouthIns(Level& lev, list<Level>::iterator it) {
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d north = Vec3d(0,0,-1);
    bool southOut = false;
    auto checkOR = [&](int a, int b){
        if ( a >= 2 && a < lev.patches.size() - 2 && b == lev.patches[a].size() - 1 + lev.shift[1] ) return true;
        return false;
    };
    if (lev.type == INNERQUAD) { return; }
    if (lev.type == OUTERRING && lev.LODlvl != currentMaxLODlvl) {
        //move patches on outer ring
        if (lev.shift[1] < 0) southOut = true;
        lev.shift[1]--;
        if (lev.shift[1] == -2) lev.shift[1]=0;
        for (int i = 0; i < lev.patches.size(); i++) {
            for (int j = 0; j < lev.patches[i].size(); j++) {
                if (checkOR(i,j)) {
                    auto patch = lev.patches[i][j];
                    patch.terrain->setTransform(lev.currentOrigin + Vec3d((i-4)*nSize+nSize*0.5,0,(-1 + lev.shift[1])*nSize+nSize*0.5));
                    ///TODO: swap textures and heightmap
                    //patch.terrain->paintHeights("data/test64x64Invert.jpg");
                }
            }
        }
        if (southOut) {
            it++;
            auto nextLvl = *it;
            shiftSouthOut(nextLvl, it);
        }
    }
    cout << " VRAtlas::need to shift SOUTH  inside: " << lev.type << " " << lev.shift[1] << endl;
}

void VRAtlas::Layout::shiftSouthOut(Level& lev, list<Level>::iterator it) {
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d north = Vec3d(0,0,-1);
    lev.currentOrigin = lev.currentOrigin - north*2*nSize;
    auto checkIR = [&](int a, int b){
        if ( a >= 2 && a < lev.patches.size() - 2 && b >= lev.patches[a].size() - 2 ) return true;
        return false;
    };

    if (lev.type == INNERQUAD) {
        //transfer patches' ownership to inner ring
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (j < 2) {
                    Patch nPatch = lev.patches[i][j];
                    innerRing.patches[i+2].insert(innerRing.patches[i+2].begin()+2,nPatch);
                }
            }
        }
        //erase patches in inner quad
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 2; j++) lev.patches[i].erase(lev.patches[i].begin());
        }
        shiftSouthOut(innerRing, it);
    }

    if (lev.type == INNERRING) {
        //transfer patches' ownership to inner quad
        for (int i = 2; i < 6; i++) {
            for (int j = 0; j < lev.patches[i].size(); j++) {
                if (checkIR(i,j)) innerQuad.patches[i-2].push_back(lev.patches[i][j]);
            }
        }
        //erase patches in inner ring
        for (int i = 2; i < 6; i++) {
            for (int j = 0; j < 2; j++) lev.patches[i].pop_back();
        }
        //move patches on inner ring
        for (int i = 0; i < lev.patches.size(); i++) { //transfer patches ownership to next bigger ring
            for (int j = 0; j < 2; j++) {
                auto patch = lev.patches[i].front();
                innerRing.patches[i].push_back(patch);
                innerRing.patches[i].erase(innerRing.patches[i].begin());
                patch.terrain->setTransform(lev.currentOrigin + Vec3d((i-4)*nSize+nSize*0.5,0,(j-4+6)*nSize+nSize*0.5));
                ///TODO: swap textures and heightmap
                //patch.terrain->paintHeights("data/test64x64Invert.jpg");
            }
        }
        auto it = levels.begin();
        it++;
        //shiftSouthIns(*it, it);
    }

    if (lev.type == OUTERRING && lev.LODlvl != currentMaxLODlvl) {
        //move patches on outer ring
        for (int i = 0; i < lev.patches.size(); i++) {
            for (int j = 0; j < 2; j++) {
                auto patch = lev.patches[i].front();
                innerRing.patches[i].push_back(patch);
                innerRing.patches[i].erase(innerRing.patches[i].begin());
                patch.terrain->setTransform(lev.currentOrigin + Vec3d((i-4)*nSize+nSize*0.5,0,(j-4+6)*nSize+nSize*0.5));
                ///TODO: swap textures and heightmap
                //patch.terrain->paintHeights("data/test64x64Invert.jpg");
            }
        }
        it++;
        auto nextLvl = *it;
        shiftSouthIns(nextLvl, it);
    }
    /*
    if (lev.type != INNERQUAD && lev.LODlvl == currentMaxLODlvl){
        vector<Patch> n1 = lev.patches.front();
        int i = 0;
        int j = 6;
        for (auto each : n1) {
            each.terrain->setTransform(lev.currentOrigin+Vec3d((i-4)*nSize+nSize*0.5,0,(j-4)*nSize+nSize*0.5));
            i++;
        }
        lev.patches.erase(lev.patches.begin());
        lev.patches.push_back(n1);
        vector<Patch> n2 = lev.patches.front();
        i = 0;
        j = 7;
        for (auto each : n2) {
            each.terrain->setTransform(lev.currentOrigin+Vec3d((i-4)*nSize+nSize*0.5,0,(j-4)*nSize+nSize*0.5));
            i++;
        }
        lev.patches.erase(lev.patches.begin());
        lev.patches.push_back(n2);
    }*/
    cout << " VRAtlas::need to shift SOUTH outside: " << lev.type << endl;
}

void VRAtlas::test() {
    cout << "VRAtlas::test" << endl;
    auto lev = layout.innerQuad;
    for (int i = 0; i < lev.patches.size(); i++) {
        for (int j = 0; j < lev.patches[i].size(); j++) {
            lev.patches[i][j].terrain->paintHeights("data/test64x64Invert.jpg");
        }
    }
}

void VRAtlas::toggleUpdater() {
    stop = !stop;
}

VRTerrainPtr VRAtlas::generateTerrain(string id, int lvl){
    string pathOrtho = "data/test64x64.jpg";
    string pathHeight = "data/testW64x64.jpg";
    string name = "testTer" + id;
    //VRTexturePtr heightIMG = loadGeoRasterData(pathHeight, false);
    VRTexturePtr heightIMG = VRTexture::create();
    float edgeLength = size*float(pow(2,lvl));
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
    if (stop) return;
    if (patchcount == 0) return;
    auto defCam = VRScene::getCurrent()->getActiveCamera();
    auto defCamPos = defCam->getWorldPosition();//defCam->getFrom();
    int tmp = layout.currentLODlvl;
    Vec3d vUp = Vec3d(0,1,0);
    Vec3d vEast = Vec3d(1,0,0);
    Vec3d vNorth = Vec3d(0,0,-1);
    Vec3d camToOrigin = defCamPos - layout.innerQuad.currentOrigin;
    float height = camToOrigin.dot(vUp);

    //float dis = (defCamPos - layout.currentOrigin).length();
    /*
    float upperbound = LODviewHeight * pow(2.0, float(tmp));
    float lowerbound = LODviewHeight * ( pow(2.0, float(tmp) -1));
    if (height > upperbound) {
        upSize();
        defCam->setNear(0.001*upperbound);
        defCam->setFar(200*upperbound);
    }
    if (height < lowerbound && tmp > 0) {
        downSize();
        defCam->setNear(0.001*lowerbound);
        defCam->setFar(200*lowerbound);
    }
    */

    auto checkShift = [&](Level& lev){
        bool shifted = false;
        Vec3d camToOrigin = defCamPos - lev.currentOrigin;
        float boundaryIns =   lev.edgeLength;
        float boundaryOut = 2*lev.edgeLength;
        float east = camToOrigin.dot(vEast);
        float north = camToOrigin.dot(vNorth);
        if (east > boundaryOut) { cout << "need shiftEastOut" << endl; layout.shiftEastOut(lev, layout.levels.begin()); }
        if (east <-boundaryOut) { cout << "need shiftWestOut" << endl; layout.shiftWestOut(lev, layout.levels.begin()); }
        if (north > boundaryOut) { cout << "need shiftNorthOut" << endl; layout.shiftNorthOut(lev, layout.levels.begin()); }
        if (north < -boundaryOut) { cout << "need shiftSouthOut" << endl; layout.shiftSouthOut(lev, layout.levels.begin()); }
        //if (east > boundaryIns || east <-boundary || north > boundary || north <-boundary) shifted = true;
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
    layout.innerRing.type = OUTERRING;
    //make new innerGrid and new innerRing
    addInnerQuad(layout.currentLODlvl);
    addInnerRing(layout.currentLODlvl);
    //cout << "VRAtlas::downSize to lvl: " << layout.currentLODlvl << " -- "<< patchcount << endl;
}

void VRAtlas::upSize() {
    layout.currentLODlvl++;
    layout.currentMaxLODlvl++;
    if (layout.currentMaxLODlvl > 4) return;
    //destroy innerGrid
    for (auto eachRow : layout.innerQuad.patches) {
        for (auto each : eachRow) layout.toDestroy.push_back(each);
    }
    for (auto eachRow : layout.innerRing.patches) {
        for (auto each : eachRow) layout.toDestroy.push_back(each);
    }
    layout.levels.pop_front(); //innerRing
    layout.innerRing = layout.levels.front();
    layout.innerRing.type = INNERRING;
    //make new innerGrid and outerRing
    addInnerQuad(layout.currentLODlvl);
    addOuterRing(layout.currentMaxLODlvl);
    //cout << "VRAtlas::upSize to lvl: " << layout.currentLODlvl << " iRs: "<< layout.innerRing.size() << " -- "<< patchcount <<  endl;
}

void VRAtlas::addInnerQuad(int lvl) {
    auto check = [&](int a, int b){
        if (a > 1 && b > 1 && a < 6 && b < 6) return true;
        return false;
    };
    Level lev = Level(INNERQUAD,lvl);
    int fac = pow ( 2, lvl );
    float nSize = size*fac;
    lev.edgeLength = nSize;
    for (int i = 0; i < 8; i++){
        vector<Patch> col;
        for (int j = 0; j < 8; j++){
            if (check(i,j)){
                string id = "LOD_" + toString(lvl) + "_" + toString(fac) + "_" + toString(i) + toString(j);
                auto ter = generateTerrain( id , lvl );
                atlas->addChild(ter);
                ter->setTransform(Vec3d((i-4)*nSize+nSize*0.5,0,(j-4)*nSize+nSize*0.5));
                Patch p = Patch(id, lvl, ter);
                p.coords = atlasOrigin + Vec2d((i-4)*nSize,(j-4)*nSize);
                col.push_back(p);
                patchcount++;
            }
        }
        if (col.size() > 0) lev.patches.push_back(col);
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
    lev.edgeLength = nSize;
    for (int i = 0; i < 8; i++){
        vector<Patch> col;
        for (int j = 0; j < 8; j++){
            if (check(i,j)){
                string id = "LOD_" + toString(lvl) + "_" + toString(fac) + "_" + toString(i) + toString(j);
                auto ter = generateTerrain( id , lvl );
                atlas->addChild(ter);
                ter->setTransform(Vec3d((i-4)*nSize+nSize*0.5,0,(j-4)*nSize+nSize*0.5));
                Patch p = Patch(id, lvl, ter);
                p.coords = atlasOrigin + Vec2d((i-4)*nSize,(j-4)*nSize);
                col.push_back(p);
                patchcount++;
            }
        }
        lev.patches.push_back(col);
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
    lev.edgeLength = nSize;
    for (int i = 0; i < 8; i++){
        vector<Patch> row;
        for (int j = 0; j < 8; j++){
            if (check(i,j)){
                string id = "LOD_" + toString(lvl) + "_" + toString(fac) + "_" + toString(i) + toString(j);
                auto ter = generateTerrain( id , lvl );
                atlas->addChild(ter);
                ter->setTransform(Vec3d((i-4)*nSize+nSize*0.5,0,(j-4)*nSize+nSize*0.5));
                Patch p = Patch(id, lvl, ter);
                p.coords = atlasOrigin + Vec2d((i-4)*nSize,(j-4)*nSize);
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

void VRAtlas::setCoordOrigin(double east, double north) { atlasOrigin = Vec2d(east,north); }
void VRAtlas::setBoundary(double minEast, double maxEast, double minNorth, double maxNorth) { bounds = Boundary(minEast, maxEast, minNorth, maxNorth); }
void VRAtlas::setServerURL(string url) { serverURL = url; }
void VRAtlas::setLocalPaths(string ortho, string height); { localPathOrtho = ortho; localPathHeight = height; }

VRTransformPtr VRAtlas::setup() {
    cout << "VRAtlas::setup" << endl;

    updatePtr = VRUpdateCb::create("atlas update", bind(&VRAtlas::update, this));
    VRScene::getCurrent()->addUpdateFkt(updatePtr);

    atlas = VRTransform::create("testTransform");

    auto geo = generatePatch("000");
    atlas->addChild(geo);

    LODMax = 3;

    addInnerQuad(0);
    addInnerRing(0);
/*
    for (int i = 0; i < LODMax; i++){
        layout.currentMaxLODlvl++;
        addOuterRing(layout.currentMaxLODlvl);
    }
*/
    //geo.setTransform(Vec3d(0,0,0));

    //Vec3d nor;
    //gdata.pushNorm(nor);

    return atlas;
}
