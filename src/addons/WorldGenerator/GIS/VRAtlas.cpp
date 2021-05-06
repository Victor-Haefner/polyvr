#include "VRAtlas.h"
#include "VRMapManager.h"

#include "core/utils/toString.h"
#include "core/scene/VRScene.h"
#include "core/utils/system/VRSystem.h"
#include "core/math/pose.h"
#include "core/objects/VRCamera.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/utils/system/VRSystem.h"

#ifndef WITHOUT_GDAL
#include "core/scene/import/GIS/VRGDAL.h"
#endif

#include <iostream>
#include <list>
#include <fstream>
#include <cmath>
#include <string>

using namespace OSG;

//template<> string typeName(const VRAtlas& p) { return "Atlas"; }

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

void VRAtlas::setMapManager(VRMapManagerPtr mgr) {
    mapMgr = mgr;
}

void VRAtlas::Patch::paint() {
    //cout << "VRAtlas::Patch::paint " << coords << " " << mapMgr << endl;
    auto onMap = [](VRMapDescriptorPtr desc, VRTerrainPtr terrain, Vec3d localPos) {
        if (!desc->isComplete()) return;

        string orthoPic = desc->getMapPath(2);
        string heightPic = desc->getMapPath(3);

        bool checkHeight = exists(heightPic);
        bool checkOrtho = exists(orthoPic);
        if ( checkHeight && checkOrtho && terrain) {
            terrain->setVisible(true);
            terrain->setHeightOffset(true);
            terrain->loadMap( heightPic, 3, false );
            terrain->paintHeights(orthoPic);//, mixColor, mixAmount );
            double localHeightoffset = terrain->getHeightOffset();
            Vec3d pos = Vec3d(localPos[0],localHeightoffset,localPos[2]);
            terrain->setTransform(pos);

            auto mt = terrain->getMaterial();
            mt->setSortKey(2);
            mt->setStencilBuffer(false, 1, -1, GL_ALWAYS, GL_KEEP, GL_KEEP, GL_KEEP);
        }
    };

    VRMapCbPtr cb = VRMapCb::create("atlasOnMap", bind(onMap, placeholders::_1, terrain, localPos));

    if (mapMgr) {
        auto mdata = mapMgr->getMap(coords[0], coords[1], edgeLength, {2,3}, cb); // NES, types, VRMapCbPtr

        /*orthoPic = mdata->getMap(2);
        heightPic = mdata->getMap(3);
        cout << "orthoPic: "  << orthoPic  << endl;
        cout << "heightPic: " << heightPic << endl;*/
    }
}

VRAtlas::Level::Level(int t, int lvl) : type(t), LODlvl(lvl) {}
VRAtlas::Level::Level() {}
VRAtlas::Level::~Level() {}

VRAtlas::Layout::Layout() {}
VRAtlas::Layout::~Layout() {}


void VRAtlas::Layout::setCoords(Patch& pat, Vec3d co3, int p_type) {
    pat.coords = coordOrigin + Vec2d(co3[0],-co3[2]);
    pat.type = p_type;
    float nSize = pat.edgeLength;

    auto cut = [&](string in) {
        int sAt = in.length();
        if (in.find(".")) {
            sAt = in.find(".");
        }
        return in.substr(0,sAt);
    };

    string pathOrtho = "data/test64x64.jpg";
    string pathHeight = "data/testW64x64.jpg";

    string east  = cut( to_string(pat.coords[0]) );
    string north = cut( to_string(pat.coords[1]) );

    Color4f mixColor = Color4f(0,1,0,0);
    float mixAmount = 1;

    string fileOrtho = "fdop20_32" + east + "_" + north + "_rgbi_" + cut(to_string(pat.edgeLength)) + ".jpg";
    string fileHeight = "dgm_E32" + east + ".5_N" + north + ".5_S"+ cut(to_string(pat.edgeLength)) + ".tif";
    //cout << fileOrtho << " --- " << fileHeight << " --- " << toString(pos) << endl;
    pat.orthoPic = localPathOrtho + "/" + cut(to_string(pat.edgeLength)) + "/" + fileOrtho;
    pat.heightPic = localPathHeight + "/" + cut(to_string(pat.edgeLength)) + "/" + fileHeight;
    //pat.recent = false;

    //if (pat.edgeLength > 2000) cout << fileOrtho << " -- " << checkOrthop << "|"<< checkHeight << " -- " << tmp1 << endl;

    /*
    pat.terrain->paintHeights(pathOrtho, mixColor, mixAmount );
    //cout << tmp1 << "-----" << tmp2 << endl;
    //if ( exists(tmp1) ) cout << "found " << tmp1 << endl; else cout << " not found " << tmp1 << endl;
    if ( checkHeight ) {
        pat.terrain->setHeightOffset(true);
        pat.terrain->loadMap( tmp2, 3, false );
        pat.localHeightoffset = pat.terrain->getHeightOffset();
    } else {
        VRTexturePtr heightIMG = VRTexture::create();
        heightIMG->read(pathHeight);
        pat.terrain->setMap( heightIMG, 3 );
    }*/
    /*if (!checkHeight && !checkOrthop) pat.terrain->setVisible(false);
    else pat.terrain->setVisible(true);
    if (debugMode) pat.terrain->setVisible(true);*/

    Vec3d pos = co3 + Vec3d(nSize*0.5,0,nSize*0.5);
    pat.localPos = pos;
}

void VRAtlas::Layout::paintAll() {
    cout << "VRAtlas::Layout::paintAll " << innerQuad.patches.size() << endl;
    for (auto patCol : innerQuad.patches) {
        for (auto& pat: patCol) pat.paint();
    }

    for (auto lev : levels) {
        for (auto patCol : lev.patches) {
            for (auto& pat : patCol) pat.paint();
        }
    }
}

void VRAtlas::Layout::debugPaint(){
    string pathOrtho = "data/test64x64.jpg";
    string pathHeight = "data/testW64x64.jpg";

    auto paint =[&](Patch& pat) {
        Color4f mixColor = Color4f(0,0,0,0);
        float mixAmount = 1;
        mixColor = Color4f(0,1,0,1);
        mixAmount = 0.2*pat.LODlvl;
        if (pat.type == OUTERRING){
            mixColor = Color4f(0,1,0,1);
            mixAmount = 0.5;
        }
        if (pat.type == INNERRING){
            mixColor = Color4f(0,0,1,1);
            mixAmount = 0.5;
        }

        if (pat.type == INNERQUAD){
            mixColor = Color4f(1,0,0,1);
            mixAmount = 0.5;
        }
        pat.terrain->paintHeights(pathOrtho, mixColor, mixAmount );
    };

    for (auto patCol : innerQuad.patches) {
        for (auto pat : patCol) paint(pat);
    }

    for (auto lev : levels) {
        for (auto patCol : lev.patches) {
            for (auto& pat : patCol) paint(pat);
        }
    }
}

void VRAtlas::Layout::reset(Vec3d camPos) {
    Vec3d pos = Vec3d(coordOrigin[0],0,coordOrigin[2]);

    //cout << "trying to reset" << endl;
    return;
    innerQuad.currentOrigin = pos;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            pos = Vec3d(0,0,0);
            auto& patch = innerQuad.patches[i][j];
            setCoords(patch,pos, INNERQUAD);
        }
    }
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < levels.front().patches[0].size(); j++) {
            pos = Vec3d(0,0,0);
            auto& patch = levels.front().patches[i][j];
            setCoords(patch,pos, INNERRING);
        }
    }
}

void VRAtlas::Layout::shiftEastIns(Level& lev, list<Level>::iterator it, bool traverse) {
    //cout << " VRAtlas::need to shift EAST inside:" << endl;
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d east = Vec3d(1,0,0);

    bool outer = false;
    if (lev.type == OUTERRING) {
        //move patches on outer ring
        lev.shift[0]++;
        if (lev.shift[0] > 1) outer = true;
        for (int j = 0; j < 4; j++) {
            auto patch = lev.patches[ 5+lev.shift[0] ][ 2+j-lev.shift[1] ];
            Vec3d nPos = lev.currentOrigin + Vec3d((-3+lev.shift[0])*nSize,0,(-2+j-lev.shift[1])*nSize);
            setCoords(patch,nPos, OUTERRING);
            lev.patches[ 1+lev.shift[0] ].insert(lev.patches[ 1+lev.shift[0] ].begin()+2+j-lev.shift[1],patch);
        }
        for (int j = 0; j < 4; j++) {
            lev.patches[ 5+lev.shift[0] ].erase(lev.patches[ 5+lev.shift[0] ].begin()+2-lev.shift[1]);
        }

        if (outer && traverse) shiftEastOut(lev, it);
    }
}

void VRAtlas::Layout::shiftEastOut(Level& lev, list<Level>::iterator it) {
    //cout << " VRAtlas::need to shift EAST outside:" << endl;
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d east = Vec3d(1,0,0);
    lev.currentOrigin = lev.currentOrigin + east*2*nSize;
    lev.coordOrigin = lev.coordOrigin + Vec2d(1,0)*2*nSize;

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
                    nPatch.type = INNERRING;
                    levels.front().patches[i+2].insert(levels.front().patches[i+2].begin()+2+j,nPatch);
                }
            }
        }
        //erase patches in inner quad
        for (int i = 0; i < 2; i++) {
            lev.patches.erase(lev.patches.begin());
        }
        shiftEastOut(levels.front(), it);
    }

    if (lev.type == INNERRING) {
        //transfer patches' ownership to inner quad
        for (int i = 6; i < 8; i++) {
            vector<Patch> col;
            for (int j = 2; j < 6; j++) {
                if (checkIR(i,j)) {
                    lev.patches[i][j].type = INNERQUAD;
                    col.push_back(lev.patches[i][j]);
                }
            }
            innerQuad.patches.push_back(col);
        }
        //erase patches in inner ring
        for (int i = 6; i < 8; i++) {
            for (int j = 2; j < 6; j++) lev.patches[i].erase(lev.patches[i].begin()+2);
        }
        //move patches on inner ring
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 8; j++){
                auto& patch = lev.patches[i][j];
                Vec3d nPos = lev.currentOrigin + Vec3d((i+6-4)*nSize,0,(j-4)*nSize);
                setCoords(patch,nPos,INNERRING);
            }
        }
        for (int i = 0; i < 2; i++) {
            auto col = lev.patches.front();
            lev.patches.push_back(col);
            lev.patches.erase(lev.patches.begin());
        }
        auto it = levels.begin();
        it++;
        shiftEastIns(*it, it);
    }

    if (lev.type == OUTERRING) {
        //move patches on outer ring
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < lev.patches.front().size(); j++) {
                auto& patch = lev.patches.front()[j];
                Vec3d nPos = lev.currentOrigin + Vec3d((2+i)*nSize,0,(j-4)*nSize);
                setCoords(patch,nPos,OUTERRING);
            }
            auto col = lev.patches.front();
            lev.patches.push_back(col);
            lev.patches.erase(lev.patches.begin());
        }
        lev.shift[0]=0;
        if (lev.LODlvl != currentMaxLODlvl) {
            it++;
            auto& nextLvl = *it;
            shiftEastIns(nextLvl, it);
        }
    }
}

void VRAtlas::Layout::shiftWestIns(Level& lev, list<Level>::iterator it, bool traverse) {
    //cout << " VRAtlas::need to shift WEST inside:" << endl;
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d east = Vec3d(1,0,0);

    bool outer = false;
    if (lev.type == OUTERRING) {
        //move patches on outer ring
        lev.shift[0]--;
        if (lev.shift[0] < -1) outer = true;
        for (int j = 0; j < 4; j++) {
            auto patch = lev.patches[ 2+lev.shift[0] ][ 2+j-lev.shift[1] ];
            Vec3d nPos = lev.currentOrigin + Vec3d((2+lev.shift[0])*nSize,0,(-2+j-lev.shift[1])*nSize);
            setCoords(patch,nPos,OUTERRING);
            lev.patches[ 6+lev.shift[0] ].insert(lev.patches[ 6+lev.shift[0] ].begin()+2+j-lev.shift[1],patch);
        }
        for (int j = 0; j < 4; j++) {
            lev.patches[ 2+lev.shift[0] ].erase(lev.patches[ 2+lev.shift[0] ].begin()+2-lev.shift[1]);
        }
        if (outer && traverse) shiftWestOut(lev, it);
    }
}

void VRAtlas::Layout::shiftWestOut(Level& lev, list<Level>::iterator it) {
    //cout << " VRAtlas::need to shift WEST outside:" << endl;
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d east = Vec3d(1,0,0);
    lev.currentOrigin = lev.currentOrigin - east*2*nSize;
    lev.coordOrigin = lev.coordOrigin - Vec2d(1,0)*2*nSize;

    if (lev.type == INNERQUAD) {
        //transfer patches' ownership to inner ring
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (i >= 2) {
                    Patch nPatch = lev.patches[i][j];
                    nPatch.type = INNERRING;
                    levels.front().patches[i+2].insert(levels.front().patches[i+2].begin()+2+j,nPatch);
                }
            }
        }
        //erase patches in inner quad
        for (int i = 2; i < 4; i++) {
            lev.patches.pop_back();
        }
        shiftWestOut(levels.front(), it);
    }

    if (lev.type == INNERRING) {
        //transfer patches' ownership to inner quad
        for (int i = 0; i < 2; i++) {
            vector<Patch> col;
            for (int j = 2; j < 6; j++) {
                lev.patches[i][j].type = INNERQUAD;
                col.push_back(lev.patches[i][j]);
            }
            innerQuad.patches.insert(innerQuad.patches.begin()+i,col);
        }
        //erase patches in inner ring
        for (int i = 0; i < 2; i++) {
            for (int j = 2; j < 6; j++) lev.patches[i].erase(lev.patches[i].begin()+2);
        }
        //move patches on inner ring
        for (int i = 7; i >= 6; i--) {
            for (int j = 0; j < 8; j++){
                auto& patch = lev.patches[i][j];
                Vec3d nPos = lev.currentOrigin + Vec3d((i-6-4)*nSize,0,(j-4)*nSize);
                setCoords(patch,nPos,INNERRING);
            }
        }
        for (int i = 0; i < 2; i++) {
            auto col = lev.patches.back();
            lev.patches.insert(lev.patches.begin(),col);
            lev.patches.pop_back();
        }
        auto it = levels.begin();
        it++;
        shiftWestIns(*it, it);
    }

    if (lev.type == OUTERRING) {
        //move patches on outer ring
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < lev.patches.back().size(); j++) {
                auto& patch = lev.patches.back()[j];
                Vec3d nPos = lev.currentOrigin + Vec3d((-3-i)*nSize,0,(j-4)*nSize);
                setCoords(patch,nPos,OUTERRING);
            }
            auto col = lev.patches.back();
            lev.patches.insert(lev.patches.begin(),col);
            lev.patches.pop_back();
        }
        lev.shift[0]=0;
        if (lev.LODlvl != currentMaxLODlvl) {
            it++;
            auto& nextLvl = *it;
            shiftWestIns(nextLvl, it);
        }
    }
}

void VRAtlas::Layout::shiftNorthIns(Level& lev, list<Level>::iterator it, bool traverse) {
    //cout << " VRAtlas::need to shift NORTH inside:" << endl;
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d north = Vec3d(0,0,-1);

    bool outer = false;
    auto checkP = [&](int a, int b){
        if ( a >= 2 + lev.shift[0]  && a < lev.patches.size() - 2 + lev.shift[0] && b == 2 - lev.shift[1] ) return true;
        return false;
    };
    if (lev.type == OUTERRING) {
        //move patches on outer ring
        lev.shift[1]++;
        if (lev.shift[1] > 1) outer = true;
        for (int i = 0; i < lev.patches.size(); i++) {
            for (int j = 0; j < lev.patches[i].size(); j++) {
                if (checkP(i,j)) {
                    auto& patch = lev.patches[i][j];
                    Vec3d nPos = lev.currentOrigin + Vec3d((i-4)*nSize,0,(2 - lev.shift[1])*nSize);
                    setCoords(patch,nPos,OUTERRING);
                }
            }
        }

        if (outer && traverse) shiftNorthOut(lev, it);
    }
}

void VRAtlas::Layout::shiftNorthOut(Level& lev, list<Level>::iterator it) {
    //cout << " VRAtlas::need to shift NORTH outside:" << endl;
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d north = Vec3d(0,0,-1);
    lev.currentOrigin = lev.currentOrigin + north*2*nSize;
    lev.coordOrigin = lev.coordOrigin + Vec2d(0,1)*2*nSize;

    auto checkIR = [&](int a, int b){
        if ( a >= 2 && a < lev.patches.size() - 2 && b < 2 ) return true;
        return false;
    };

    if (lev.type == INNERQUAD) {
        //transfer patches' ownership to inner ring
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (j >= 2) {
                    auto nPatch = lev.patches[i][j];
                    nPatch.type = INNERRING;
                    levels.front().patches[i+2].insert(levels.front().patches[i+2].begin()+2,nPatch);
                }
            }
        }
        //erase patches in inner quad
        for (int i = 0; i < 4; i++) {
            for (int j = 2; j < 4; j++) lev.patches[i].erase(lev.patches[i].end());
        }
        shiftNorthOut(levels.front(), it);
    }

    if (lev.type == INNERRING) {
        //transfer patches' ownership to inner quad
        for (int i = 2; i < 6; i++) {
            for (int j = 2; j >= 0; j--) {
                if (checkIR(i,j)) {
                    lev.patches[i][j].type = INNERQUAD;
                    innerQuad.patches[i-2].insert(innerQuad.patches[i-2].begin(),lev.patches[i][j]);
                }
            }
        }
        //erase patches in inner ring
        for (int i = 2; i < 6; i++) {
            for (int j = 0; j < 2; j++) lev.patches[i].erase(lev.patches[i].begin());
        }
        //move patches on inner ring
        for (int i = 0; i < lev.patches.size(); i++) {
            for (int j = 1; j >= 0; j--) {
                auto patch = lev.patches[i].back();
                lev.patches[i].insert(lev.patches[i].begin(),patch);
                lev.patches[i].pop_back();
                Vec3d nPos = lev.currentOrigin + Vec3d((i-4)*nSize,0,(j-4)*nSize);
                auto& nPatch = lev.patches[i].front();
                setCoords(nPatch,nPos,INNERQUAD);
            }
        }
        auto it = levels.begin();
        it++;
        shiftNorthIns(*it, it);
    }

    if (lev.type == OUTERRING) {
        //move patches on outer ring
        for (int i = 0; i < lev.patches.size(); i++) {
            for (int j = 1; j >= 0; j--) {
                auto patch = lev.patches[i].back();
                lev.patches[i].pop_back();
                lev.patches[i].insert(lev.patches[i].begin(),patch);
                Vec3d nPos = lev.currentOrigin + Vec3d((i-4)*nSize,0,(j-4)*nSize);
                auto& nPatch = lev.patches[i].front();
                setCoords(nPatch,nPos,OUTERRING);
            }
        }
        lev.shift[1]=0;
        if (lev.LODlvl != currentMaxLODlvl) {
            it++;
            auto& nextLvl = *it;
            shiftNorthIns(nextLvl, it);
        }
    }
}

void VRAtlas::Layout::shiftSouthIns(Level& lev, list<Level>::iterator it, bool traverse) {
    //cout << " VRAtlas::need to shift SOUTH  inside: " << lev.type << " " << lev.shift[1] << endl;
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d north = Vec3d(0,0,-1);
    bool outer = false;
    auto checkOR = [&](int a, int b){
        if ( a >= 2 + lev.shift[0] && a < lev.patches.size() - 2 + lev.shift[0] && b == 1 - lev.shift[1] ) return true;
        return false;
    };
    if (lev.type == OUTERRING) {
        //move patches on outer ring
        lev.shift[1]--;
        if (lev.shift[1] < -1) outer = true;
        for (int i = 0; i < lev.patches.size(); i++) {
            for (int j = 0; j < lev.patches[i].size(); j++) {
                if (checkOR(i,j)) {
                    auto& patch = lev.patches[i][j];
                    Vec3d nPos = lev.currentOrigin + Vec3d((i-4)*nSize,0,(-3 - lev.shift[1])*nSize);
                    setCoords(patch,nPos,OUTERRING);
                }
            }
        }
        if (outer && traverse) shiftSouthOut(lev, it);
    }
}

void VRAtlas::Layout::shiftSouthOut(Level& lev, list<Level>::iterator it) {
    //cout << " VRAtlas::need to shift SOUTH outside: " << lev.type << endl;
    int lvl = lev.LODlvl;
    float nSize = lev.edgeLength;
    Vec3d north = Vec3d(0,0,-1);
    lev.currentOrigin = lev.currentOrigin - north*2*nSize;
    lev.coordOrigin = lev.coordOrigin - Vec2d(0,1)*2*nSize;
    auto checkIR = [&](int a, int b){
        if ( a >= 2 && a < lev.patches.size() - 2 && b >= lev.patches[a].size() - 2 ) return true;
        return false;
    };

    if (lev.type == INNERQUAD) {
        //transfer patches' ownership to inner ring
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (j < 2) {
                    auto nPatch = lev.patches[i][j];
                    nPatch.type = INNERRING;
                    auto& lll = *it;
                    lll.patches[i+2].insert( lll.patches[i+2].begin()+2,nPatch );
                }
            }
        }
        //erase patches in inner quad
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 2; j++) lev.patches[i].erase(lev.patches[i].begin());
        }
        shiftSouthOut(levels.front(), it);
    }

    if (lev.type == INNERRING) {
        //transfer patches' ownership to inner quad
        for (int i = 2; i < 6; i++) {
            for (int j = 0; j < lev.patches[i].size(); j++) {
                if (checkIR(i,j)) {
                    lev.patches[i][j].type = INNERQUAD;
                    innerQuad.patches[i-2].push_back(lev.patches[i][j]);
                }
            }
        }
        //erase patches in inner ring
        for (int i = 2; i < 6; i++) {
            for (int j = 0; j < 2; j++) lev.patches[i].pop_back();
        }
        //move patches on inner ring
        for (int i = 0; i < lev.patches.size(); i++) {
            for (int j = 0; j < 2; j++) {
                auto patch = lev.patches[i].front();
                lev.patches[i].push_back(patch);
                lev.patches[i].erase(lev.patches[i].begin());
                Vec3d nPos = lev.currentOrigin + Vec3d((i-4)*nSize,0,(j-4+6)*nSize);
                auto& nPatch = lev.patches[i].back();
                setCoords(nPatch,nPos,INNERRING);
            }
        }
        auto it = levels.begin();
        it++;
        shiftSouthIns(*it, it);
    }

    if (lev.type == OUTERRING) {
        //move patches on outer ring
        for (int i = 0; i < lev.patches.size(); i++) {
            for (int j = 0; j < 2; j++) {
                auto patch = lev.patches[i].front();
                lev.patches[i].erase(lev.patches[i].begin());
                lev.patches[i].push_back(patch);
                Vec3d nPos = lev.currentOrigin + Vec3d((i-4)*nSize,0,(j+2)*nSize);
                auto& nPatch = lev.patches[i].back();
                setCoords(nPatch,nPos,OUTERRING);
            }
        }
        lev.shift[1]=0;
        if (lev.LODlvl != currentMaxLODlvl) {
            it++;
            auto& nextLvl = *it;
            shiftSouthIns(nextLvl, it);
        }
    }
}

void VRAtlas::test() {
    cout << "VRAtlas::test" << endl;
    auto lev = layout.innerQuad;
    /*for (int i = 0; i < lev.patches.size(); i++) {
        for (int j = 0; j < lev.patches[i].size(); j++) {
            lev.patches[i][j].terrain->paintHeights("data/test64x64Invert.jpg");
        }
    }*/
    cout << " innerQuad: " << toString( layout.innerQuad.currentOrigin ) << " shift: " << toString( layout.innerQuad.shift ) << endl;
    cout << " innerRing: " << toString( layout.levels.front().currentOrigin ) << " shift: " << toString( layout.levels.front().shift ) << endl;

    string ff = " ";
    for (auto& each : layout.levels) {
        ff += " ";
        cout << ff  << each.LODlvl << "--" << each.type << "--" << toString( each.currentOrigin ) <<  " shift: " << toString( each.shift ) << " EL: " << toString( each.edgeLength ) << endl;
    }
}

void VRAtlas::toggleUpdater() {
    stop = !stop;
}

void VRAtlas::resetJobQueue() {
    patchQueue.clear();
    for (auto patCol : layout.innerQuad.patches) {
        for (auto pat: patCol) patchQueue.push_back(pat);
    }

    for (auto lev : layout.levels) {
        for (auto patCol : lev.patches) {
            for (auto pat : patCol) patchQueue.push_back(pat);
        }
    }
}

void VRAtlas::handleJobQueue() {
    int patchesPerJob = 3;
    for (int i = 0; i < patchesPerJob; i++) {
        if (patchQueue.size() > 0) {
            patchQueue.front().paint();
            patchQueue.pop_front();
        }
    }
    //repaint();
}

VRTerrainPtr VRAtlas::generateTerrain(string id, int lvl){
    string pathOrtho = "data/test64x64.jpg";
    string pathHeight = "data/testW64x64.jpg";
    string name = "Terrain_" + id;
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
    string name = "Geo_" + id;
    Vec3d origin = Vec3d(0,0,0);
    Vec3d pos0 = origin;
    float l = 500;
    Vec3d pos1 = origin + Vec3d(1,0,0);
    Vec3d pos2 = origin + Vec3d(0,l,0);
    Vec3d pos3 = origin + Vec3d(1,l,0);

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
    auto defCam = VRScene::getCurrent()->getActiveCamera();
    auto defCamPos = defCam->getWorldPosition();//defCam->getFrom();
    int tmp = layout.currentLODlvl;
    Vec3d vUp = Vec3d(0,1,0);
    Vec3d vEast = Vec3d(1,0,0);
    Vec3d vNorth = Vec3d(0,0,-1);
    //auto atlasPos = atlas->getWorldPosition();
    //Vec3d camToOrigin = defCamPos - (atlasPos+layout.innerQuad.currentOrigin);
    Vec3d camToOrigin = defCamPos - (layout.innerQuad.currentOrigin*scaling);
    float height = camToOrigin.dot(vUp);

    if (stop) return;
    if (debugMode) debugQuad->setFrom(layout.innerQuad.currentOrigin);
    float dis = (defCamPos - (layout.innerQuad.currentOrigin*scaling)).length();
    float east = camToOrigin.dot(vEast);
    float north = camToOrigin.dot(vNorth);
    if (Vec2d(east, north).length() > 2*layout.levels.back().edgeLength) layout.reset(defCamPos);

    bool shifted = false;

    auto needsShift = [&](Level& lev) {
        Vec3d camToOrigin = defCamPos - (lev.currentOrigin*scaling);
        float boundaryOut = 2*lev.edgeLength*scaling;
        float east = camToOrigin.dot(vEast);
        float north = camToOrigin.dot(vNorth);
        if (east > boundaryOut || east <-boundaryOut || north > boundaryOut || north < -boundaryOut) return true;
        return false;
    };

    auto checkShift = [&](Level& lev) {
        Vec3d camToOrigin = defCamPos - (lev.currentOrigin*scaling);
        float boundaryOut = 2*lev.edgeLength*scaling;
        float east = camToOrigin.dot(vEast);
        float north = camToOrigin.dot(vNorth);
        if (east >  boundaryOut)  { layout.shiftEastOut(lev,  layout.levels.begin()); }
        if (east < -boundaryOut)  { layout.shiftWestOut(lev,  layout.levels.begin()); }
        if (north >  boundaryOut) { layout.shiftNorthOut(lev, layout.levels.begin()); }
        if (north < -boundaryOut) { layout.shiftSouthOut(lev, layout.levels.begin()); }
        if (east > boundaryOut || east < -boundaryOut || north > boundaryOut || north < -boundaryOut) shifted = true;
    };

    checkShift(layout.innerQuad);
    layout.steady = !needsShift(layout.innerQuad);
    if (layout.steady) sinceLastMovement++;
    if (shifted) {
        sinceLastMovement = 0;
        resetJobQueue();
    }
    if (shifted) return;

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

    if (layout.toDestroy.size() > 0){
        for (auto &each : layout.toDestroy){
            each.terrain->destroy();
            patchcount--;
        }
        layout.toDestroy.clear();
    }*/

    //cout << tmp << "---"  << dis << "---"  << 500.0 * pow(2.0, float(tmp)) << "---"  << 500.0 * ( pow(2.0, float(tmp)) -1) << "---" << endl;

    //cout << defCamPos << endl;
    //if (sinceLastMovement == 60) fillQueue();
    if (sinceLastMovement > 20) handleJobQueue();
    if (debugMode) layout.debugPaint();
}

void VRAtlas::downSize() {
    layout.currentLODlvl--;
    layout.currentMaxLODlvl--;
    //destroy innerGrid
    for (auto eachRow : layout.innerQuad.patches) {
        for (auto& each : eachRow) layout.toDestroy.push_back(each);
    }
    for (auto eachRow : layout.levels.back().patches) {
        for (auto& each : eachRow) layout.toDestroy.push_back(each);
    }
    layout.levels.pop_back();
    layout.levels.front().type = OUTERRING;
    if (debugMode) for (auto& patCol : layout.levels.front().patches) for (auto& pat : patCol) pat.type = OUTERRING;
    Vec2d nOriginQuad = layout.levels.front().coordOrigin;
    Vec2d nOriginRing = layout.levels.front().coordOrigin;
    //make new innerGrid and new innerRing
    addInnerQuad(layout.currentLODlvl, nOriginQuad);
    addInnerRing(layout.currentLODlvl, nOriginRing);
    //cout << "VRAtlas::downSize to lvl: " << layout.currentLODlvl << " -- "<< patchcount << " COORDS: "<< toString(nOriginQuad) << endl;
}

void VRAtlas::upSize() {
    layout.currentLODlvl++;
    layout.currentMaxLODlvl++;
    //destroy innerGrid
    for (auto eachRow : layout.innerQuad.patches) {
        for (auto& each : eachRow) layout.toDestroy.push_back(each);
    }
    for (auto eachRow : layout.levels.front().patches) {
        for (auto& each : eachRow) layout.toDestroy.push_back(each);
    }
    layout.levels.pop_front(); //innerRing
    //cout << "SHIFT_befor: " << toString(layout.levels.front().shift) << " Vec: "<< toString(layout.levels.front().currentOrigin) << endl;
    if (layout.levels.front().shift[0] == -1) layout.shiftEastIns(layout.levels.front(), layout.levels.begin(), false);
    if (layout.levels.front().shift[0] ==  1) layout.shiftWestIns(layout.levels.front(), layout.levels.begin(), false);
    if (layout.levels.front().shift[1] == -1) layout.shiftNorthIns(layout.levels.front(), layout.levels.begin(), false);
    if (layout.levels.front().shift[1] ==  1) layout.shiftSouthIns(layout.levels.front(), layout.levels.begin(), false);
    //cout << "SHIFT_after: " << toString(layout.levels.front().shift) << " Vec: "<< toString(layout.levels.front().currentOrigin) << endl;
    layout.levels.front().type = INNERRING;
    if (debugMode) for (auto& patCol : layout.levels.front().patches) for (auto& pat : patCol) pat.type = INNERRING;

    Vec2d nOriginInner = layout.levels.front().coordOrigin;
    Vec2d nOriginOuter = layout.levels.back().coordOrigin;
    //cout << "SHIFT_befor OUTER: " << toString(layout.levels.back().shift) << " Vec: "<< toString(layout.levels.back().currentOrigin) << endl;
    //make new innerGrid and outerRing
    addInnerQuad(layout.currentLODlvl, nOriginInner);
    addOuterRing(layout.currentMaxLODlvl, nOriginOuter);
    //cout << "SHIFT_after OUTER: " << toString(layout.levels.back().shift) << " Vec: "<< toString(layout.levels.back().currentOrigin) << endl;
    //cout << "VRAtlas::upSize to lvl: " << layout.currentLODlvl << " -- "<< patchcount << " COORDS: "<< toString(nOriginInner) << " Vec: "<< toString(layout.innerQuad.currentOrigin) <<  endl;
}

void VRAtlas::addInnerQuad(int lvl, Vec2d nOrigin) {
    auto check = [&](int a, int b){
        if (a > 1 && b > 1 && a < 6 && b < 6) return true;
        return false;
    };
    Level lev = Level(INNERQUAD,lvl);
    lev.coordOrigin = nOrigin;
    lev.currentOrigin = Vec3d(nOrigin[0],0,-nOrigin[1]) - Vec3d(atlasOrigin[0],0,-atlasOrigin[1]); ///TODO: include real coordinates along planet surface here
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
                Patch p = Patch(id, lvl, ter);
                p.mapMgr = mapMgr;
                p.coords = nOrigin + Vec2d((i-4)*nSize,-(j-4)*nSize);
                p.edgeLength = nSize;
                Vec2d pRelative = p.coords - atlasOrigin;
                Vec3d pos = Vec3d(pRelative[0],0,-pRelative[1]);

                layout.setCoords(p,pos,INNERQUAD);
                col.push_back(p);
                patchcount++;
            }
        }
        if (col.size() > 0) lev.patches.push_back(col);
    }
    layout.innerQuad = lev;
    //cout << 8*nSize << "m InnerQuad" << endl;
}

void VRAtlas::addInnerRing(int lvl, Vec2d nOrigin) {
    auto check = [&](int a, int b){
        if (a < 2) return true;
        if (b < 2) return true;
        if (a > 5) return true;
        if (b > 5) return true;
        return false;
    };
    Level lev = Level(INNERRING,lvl);
    lev.coordOrigin = nOrigin;
    lev.currentOrigin = Vec3d(nOrigin[0],0,-nOrigin[1]) - Vec3d(atlasOrigin[0],0,-atlasOrigin[1]); ///TODO: include real coordinates along planet surface here
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
                Patch p = Patch(id, lvl, ter);
                p.mapMgr = mapMgr;
                p.coords = nOrigin + Vec2d((i-4)*nSize,-(j-4)*nSize);
                p.edgeLength = nSize;
                Vec2d pRelative = p.coords - atlasOrigin;
                Vec3d pos = Vec3d(pRelative[0],0,-pRelative[1]);

                layout.setCoords(p,pos,INNERRING);
                col.push_back(p);
                patchcount++;
            }
        }
        lev.patches.push_back(col);
    }
    layout.levels.push_front(lev);
    //cout << 8*nSize << "m InnerRing " << endl;
}

void VRAtlas::addOuterRing(int lvl, Vec2d nOrigin) {
    auto check = [&](int a, int b){
        if (a < 2) return true;
        if (b < 2) return true;
        if (a > 5) return true;
        if (b > 5) return true;
        return false;
    };

    Level lev = Level(OUTERRING,lvl);
    lev.coordOrigin = nOrigin;
    lev.currentOrigin = Vec3d(nOrigin[0],0,-nOrigin[1]) - Vec3d(atlasOrigin[0],0,-atlasOrigin[1]); ///TODO: include real coordinates along planet surface here
    int fac = pow ( 2, lvl );
    float nSize = size*fac;
    lev.edgeLength = nSize;
    for (int i = 0; i < 8; i++){
        vector<Patch> row;
        for (int j = 0; j < 8; j++){
            if (check(i,j)) {
                string id = "LOD_" + toString(lvl) + "_" + toString(fac) + "_" + toString(i) + toString(j);
                auto ter = generateTerrain( id , lvl );
                atlas->addChild(ter);
                Patch p = Patch(id, lvl, ter);
                p.mapMgr = mapMgr;
                p.coords = nOrigin + Vec2d((i-4)*nSize,-(j-4)*nSize);
                p.edgeLength = nSize;
                Vec2d pRelative = p.coords - atlasOrigin;
                Vec3d pos = Vec3d(pRelative[0],0,-pRelative[1]);

                layout.setCoords(p,pos,OUTERRING);
                row.push_back(p);
                patchcount++;
            }
        }
        lev.patches.push_back(row);
    }
    layout.levels.push_back(lev);
    //cout << 8*nSize << "m OuterRing" << endl;
}

void VRAtlas::setCoordOrigin(double east, double north) { atlasOrigin = Vec2d(east,north); }
void VRAtlas::setBoundary(double minEast, double maxEast, double minNorth, double maxNorth) { bounds = Boundary(minEast, maxEast, minNorth, maxNorth); }
void VRAtlas::setServerURL(string url) { serverURL = url; }
void VRAtlas::setLocalPaths(string ortho, string height) { localPathOrtho = ortho; localPathHeight = height; }
void VRAtlas::setDebug(bool mode) { debugMode = mode; layout.debugMode = mode; }
void VRAtlas::setScale(float s) { scaling = s; if (atlas) atlas->setScale(Vec3d(s,s,s)); }
void VRAtlas::repaint() { layout.paintAll(); }

Vec3d VRAtlas::getLocalPos(double east, double north) {
    Vec3d pos = Vec3d(east - atlasOrigin[0],0, - (north - atlasOrigin[1]) );
    return pos;
}

VRTransformPtr VRAtlas::setup() {
    cout << "VRAtlas::setup" << endl;

    debugQuad = generatePatch("innerQuadOrigin");
    if (!debugMode) debugQuad->setVisible(false);

    updatePtr = VRUpdateCb::create("atlas update", bind(&VRAtlas::update, this));
    VRScene::getCurrent()->addUpdateFkt(updatePtr);

    atlas = VRTransform::create("AtlasTransform");
    atlas->addChild(debugQuad);
    atlas->setScale(Vec3d(scaling, scaling, scaling));

    layout.localPathOrtho = localPathOrtho;
    layout.localPathHeight = localPathHeight;
    layout.coordOrigin = atlasOrigin;

    LODMax = 3;

    addInnerQuad(0, atlasOrigin);
    addInnerRing(0, atlasOrigin);

    for (int i = 0; i < LODMax; i++){
        layout.currentMaxLODlvl++;
        addOuterRing(layout.currentMaxLODlvl, atlasOrigin);
    }

    return atlas;
}
