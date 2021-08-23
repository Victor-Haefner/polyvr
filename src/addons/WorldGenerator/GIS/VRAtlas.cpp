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

VRAtlas::Patch::Patch(string sid, int lvl) : id(sid), LODlvl(lvl) {}
VRAtlas::Patch::Patch() {}
VRAtlas::Patch::~Patch() {}

void VRAtlas::setMapManager(VRMapManagerPtr mgr) {
    mapMgr = mgr;
}

void VRAtlas::Patch::paint() {
    //cout << "VRAtlas::Patch::paint " << coords << " " << mapMgr << endl;

    string pathOrtho = "data/test64x64.jpg";
    string pathHeight = "data/testW64x64.jpg";
    auto pain =[&]() {
        Color4f mixColor = Color4f(0,0,0,0);
        float mixAmount = 1;
        mixColor = Color4f(0,1,0,1);
        mixAmount = 0.2*LODlvl;
        if (LODlvl < 4){
            mixColor = Color4f(0,1,0,1);
            mixAmount = 0.5;
        }

        if (LODlvl == 4){
            mixColor = Color4f(1,0,0,1);
            mixAmount = 0.5;
        }
        terrain->paintHeights(pathOrtho, mixColor, mixAmount );
        terrain->setTransform(localPos);
        visible = true;
    };

    auto onMap = [](VRMapDescriptorPtr desc, VRTerrainPtr terrain, Vec3d localPos, bool& loaded) {
        if (!desc->isComplete()) return;

        string orthoPic = desc->getMapPath(2);
        string heightPic = desc->getMapPath(3);

        bool checkHeight = exists(heightPic);
        bool checkOrtho = exists(orthoPic);
        terrain->setVisible(true);
        if (loaded) return;
        if ( checkHeight && checkOrtho && terrain) {
            terrain->setHeightOffset(true);
            terrain->loadMap( heightPic, 0, false );
            terrain->paintHeights(orthoPic);//, mixColor, mixAmount );
            double localHeightoffset = terrain->getHeightOffset();
            Vec3d pos = Vec3d(localPos[0],localHeightoffset,localPos[2]);
            terrain->setTransform(pos);

            auto mt = terrain->getMaterial();
            mt->setSortKey(2);
            mt->setStencilBuffer(false, 1, -1, GL_ALWAYS, GL_KEEP, GL_KEEP, GL_KEEP);

            //removeFile(orthoPic);
            //removeFile(heightPic);
        }
        loaded = true;
    };

    VRMapCbPtr cb = VRMapCb::create("atlasOnMap", bind(onMap, placeholders::_1, terrain, localPos, loaded));

    if (mapMgr) {
        if (!loaded) auto mdata = mapMgr->getMap(coords[0], coords[1], edgeLength, {2,3}, cb); // NES, types, VRMapCbPtr

        /*orthoPic = mdata->getMap(2);
        heightPic = mdata->getMap(3);
        cout << "orthoPic: "  << orthoPic  << endl;
        cout << "heightPic: " << heightPic << endl;*/
    } else {
        if ( terrain) {
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
            } else {
                terrain->setVisible(false);
            }
        }
    }
}

void VRAtlas::setCoords(Patch& pat) {
    float nSize = pat.edgeLength;

    string pathOrtho = "data/test64x64.jpg";
    string pathHeight = "data/testW64x64.jpg";

    string east  = pat.east;
    string north = pat.north;
    string els = pat.els;

    Color4f mixColor = Color4f(0,1,0,0);
    float mixAmount = 1;

    string fileOrtho = "fdop20_32" + east + "_" + north + "_rgbi_" + els + ".jpg";
    string fileHeight = "dgm_E32" + east + ".5_N" + north + ".5_S"+ els + ".tif";
    //cout << fileOrtho << " --- " << fileHeight << " --- " << toString(pos) << endl;
    pat.orthoPic = localPathOrtho + "/" + els + "/" + fileOrtho;
    pat.heightPic = localPathHeight + "/" + els + "/" + fileHeight;

    Vec3d pos = Vec3d( pat.coords[0]-atlasOrigin[0], 0, - (pat.coords[1]-atlasOrigin[1]) ) + Vec3d(nSize*0.5,0,nSize*0.5);
    /*
    pat.terrain->paintHeights(pathOrtho, mixColor, mixAmount );
    //cout << tmp1 << "-----" << tmp2 << endl;
    //if ( exists(tmp1) ) cout << "found " << tmp1 << endl; else cout << " not found " << tmp1 << endl;
    if ( checkHeight ) {
        pat.terrain->setHeightOffset(true);
        pat.terrain->loadMap( tmp2, 0, false );
        pat.localHeightoffset = pat.terrain->getHeightOffset();
    } else {
        VRTexturePtr heightIMG = VRTexture::create();
        heightIMG->read(pathHeight);
        pat.terrain->setMap( heightIMG, 3 );
    }*/
    /*if (!checkHeight && !checkOrthop) pat.terrain->setVisible(false);
    else pat.terrain->setVisible(true);
    if (debugMode) pat.terrain->setVisible(true);*/

    //Vec3d pos = co3 + Vec3d(nSize*0.5,0,nSize*0.5);
    pat.localPos = pos;
}

void VRAtlas::debugPaint(){
    string pathOrtho = "data/test64x64.jpg";
    string pathHeight = "data/testW64x64.jpg";

    auto paint =[&](Patch& pat) {
        Color4f mixColor = Color4f(0,0,0,0);
        float mixAmount = 1;
        mixColor = Color4f(0,1,0,1);
        mixAmount = 0.2*pat.LODlvl;
        if (pat.LODlvl < LODMax){
            mixColor = Color4f(0,1,0,1);
            mixAmount = 0.5;
        }
        if (pat.LODlvl == LODMax){
            mixColor = Color4f(0,0,1,1);
            mixAmount = 0.5;
        }

        if (pat.LODlvl == LODMax){
            mixColor = Color4f(1,0,0,1);
            mixAmount = 0.5;
        }
        pat.terrain->paintHeights(pathOrtho, mixColor, mixAmount );
    };
}

void VRAtlas::test() {
    cout << "VRAtlas::test " << patchQueue.size() << " "<< allPatchesByID.size() << endl;
}

void VRAtlas::toggleUpdater() {
    stop = !stop;
}

void VRAtlas::resetJobQueue() {
    patchQueue.clear();
}

void VRAtlas::handleJobQueue() {
    int patchesPerJob = 10;// patchQueue.size();
    if (patchQueue.size()>10000) resetJobQueue();

    auto checkInBound = [&](string sID){
        bool res = true;
        float east = allPatchesByID[sID].coords[0];
        float north = allPatchesByID[sID].coords[1];
        if (east < bounds.minEast || east > bounds.maxEast || north < bounds.minNorth || north > bounds.maxNorth ) return false;
        return res;
    };

    for (int i = 0; i < patchesPerJob; i++) {
        if (patchQueue.size() > 0) {
            string sID = patchQueue.front();

            if (allPatchesByID[sID].edgeLength >= 25600) {
                if (allPatchesByID[sID].terrain) allPatchesByID[sID].terrain->setVisible(0);
                allPatchesByID[sID].visible = false;
                return;
            }
            if (!checkInBound(sID)) return;
            if (!allPatchesByID[sID].terrain) {
                string name = "Terrain_" + sID;
                float edgeLength = allPatchesByID[sID].edgeLength;
                Vec2d terrainSize = Vec2d(edgeLength,edgeLength);
                float scale = float(pow(2,allPatchesByID[sID].LODlvl));
                auto terrain = VRTerrain::create(name);
                terrain->deactivateVertexFlip();
                terrain->setParameters (terrainSize, scale, 1);
                allPatchesByID[sID].terrain = terrain;
                atlas->addChild(terrain);
                allPatchesByID[patchQueue.front()].paint();
            } else {
                patchesPerJob++;
            }
            if (allPatchesByID[patchQueue.front()].visibleToBe) {
                allPatchesByID[sID].terrain->setVisible(1);
                allPatchesByID[sID].visible = true;
            } else {
                allPatchesByID[sID].terrain->setVisible(0);
                allPatchesByID[sID].visible = false;
            }
            patchQueue.pop_front();
        }
    }
    int patchesHidden = 0;
    for (int i = 0; i < invisQueue.size(); i++){
        string sID = invisQueue[i];
        if (patchesHidden < patchesPerJob){
            if (allPatchesByID[sID].allowedToDeload) {
                allPatchesByID[sID].terrain->setVisible(0);
                allPatchesByID[sID].visible = false;
            }
            patchesHidden++;
        }
    }
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
    float scale = float(pow(2,lvl));
    auto terrain = VRTerrain::create(name);
    terrain->setParameters (terrainSize, scale, 1);
    terrain->paintHeights( pathOrtho, mixColor, mixAmount );
    terrain->setMap( heightIMG, 3 );

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
    Vec3d camPos = defCam->getWorldPosition();

    Vec3d vUp = Vec3d(0,1,0);
    Vec3d vEast = Vec3d(1,0,0);
    Vec3d vNorth = Vec3d(0,0,-1);
    Vec3d camToAtlas = camPos - atlas->getWorldPose()->pos();
    float height = camToAtlas.dot(vUp);
    float disEast = camToAtlas.dot(vEast)/scaling;
    float disNorth = camToAtlas.dot(vNorth)/scaling;
    //if ( checkOutOfBounds() ) return;

    int minLodLvl = int( log((abs(height)/scaling)/LODviewHeight)/log(2) );
    if (minLodLvl < 0) minLodLvl = 0;
    int maxLodLvl = minLodLvl + LODMax;

    if (minLodLvl > 6) {
        toBeReset = true;
        return;
    } else {
        if (toBeReset) {
            resetJobQueue();
            toBeReset = false;
        }
    }

    auto checkInBound = [&](Vec2d cor2){
        bool res = true;
        float east = cor2[0];
        float north = cor2[1];
        if (east < bounds.minEast || east > bounds.maxEast || north < bounds.minNorth || north > bounds.maxNorth ) return false;
        return res;
    };

    auto cut = [&](string in) {
        int sAt = in.length();
        if (in.find(".")) {
            sAt = in.find(".");
        }
        return in.substr(0,sAt);
    };

    auto toCoords = [&](string in){
        Vec2d res = Vec2d(0,0);
        int sAt = in.find("|");
        string a = in.substr(0,sAt);
        string b = in.substr(sAt+1);
        if (b.find("|")) b = b.substr(b.find("|")+1);
        res = Vec2d ( stof(a), stof(b) );
        return res;
    };

    auto cts = [&](float in){
        return cut( to_string(in));
    };

    auto split = [&](string in){
        vector<string> res;
        int sAt = in.find("|");
        string a = in.substr(0,sAt);
        string b = in.substr(sAt+1);
        int sAt2 = b.find("|");
        string c = b.substr(0,sAt2);
        string d = b.substr(sAt2+1);
        res.push_back(a);
        res.push_back(c);
        res.push_back(d);
        return res;
    };

    map<string,vector<int>> lvls;
    map<string,bool> patchesToGenerate;
    map<string,bool> highLevel;
    visiblePatchesByID.clear();
    levelPatchesByID.clear();
    auto makeGrid = [&](int lodLVL) {
        float edgeLength = pow(2,lodLVL)*size;
        Vec2d vecat = Vec2d(floor(disEast/(edgeLength)),floor(disNorth/(edgeLength)+1)); //int level origin
        if (fmod(vecat[0],2.0)!=0) vecat[0]+=1.0;
        if (fmod(vecat[1],2.0)!=0) vecat[1]-=1.0;

        for (int i = 0; i < 8; i++){
            for (int j = 0; j < 8; j++) {
                float coordE = (vecat[0]-4+j)*edgeLength+atlasOrigin[0];
                float coordN = (vecat[1]+4-i)*edgeLength+atlasOrigin[1];
                float coordx =   (vecat[0]-4+j)*edgeLength;
                //float coordz = - (vecat[1]+4-i)*edgeLength;
                float coordz =   (vecat[1]-4+i)*edgeLength;

                Vec2d coords = Vec2d(coordE, coordN);

                string east  = cut( to_string(coords[0]) );
                string north = cut( to_string(coords[1]) );
                string els  = cut( to_string(edgeLength));

                string coordsString = east + "|" + north;
                string sID = coordsString +  "|" + els;
                if (lvls.count(coordsString)) lvls[coordsString].push_back(lodLVL);
                else {
                    vector<int> res;
                    res.push_back(lodLVL);
                    lvls[coordsString] = res;
                }
                if (!visiblePatchesByID.count(sID)) visiblePatchesByID[sID] = -1;
                if (!levelPatchesByID.count(sID)) levelPatchesByID[sID] = lodLVL;
                if (lodLVL == maxLodLvl) highLevel[sID] = true;

                if (!allPatchesByID.count(sID) && checkInBound(coords)) {
                    Patch p = Patch(sID, lodLVL);
                    p.mapMgr = mapMgr;
                    p.coords = coords;
                    p.edgeLength = edgeLength;
                    p.east = east;
                    p.north = north;
                    p.els = els;
                    string fileOrtho = "fdop20_32" + east + "_" + north + "_rgbi_" + els + ".jpg";
                    string fileHeight = "dgm_E32" + east + ".5_N" + north + ".5_S"+ els + ".tif";
                    p.orthoPic = localPathOrtho + "/" + els + "/" + fileOrtho;
                    p.heightPic = localPathHeight + "/" + els + "/" + fileHeight;
                    p.localPos = Vec3d( coords[0]-atlasOrigin[0], 0, - (coords[1]-atlasOrigin[1]) ) + Vec3d(edgeLength*0.5,0,edgeLength*0.5); //Vec3d(coordx, 0, coordz);
                    allPatchesByID[sID] = p;
                }
            }
        }
    };

    for (int i = minLodLvl; i <= maxLodLvl; i++) {
        makeGrid(i);
    }

    for (auto each : lvls) {
        if (each.second.size() > 1) {
            Vec2d coords = toCoords(each.first);

            for (int i = each.second.size()-1; i > 0; i--){
                float edgeLength = pow(2,each.second[i-1])*size;
                string els = cts( edgeLength ); //edgelengthstring
                string ID1 = cts( coords[0] ) + "|" + cts( coords[1] ) + "|" + els;
                string ID2 = cts( coords[0] + edgeLength ) + "|" + cts( coords[1] ) + "|" +  els;
                string ID3 = cts( coords[0] ) + "|" + cts( coords[1] - edgeLength ) + "|" +  els;
                string ID4 = cts( coords[0] + edgeLength ) + "|" + cts( coords[1] - edgeLength ) + "|" +  els;
                vector<string> children = {ID1, ID2, ID3, ID4};

                string elsB = cts( pow(2,each.second[i])*size );
                string IDtoDisable = cts( coords[0] ) + "|" + cts( coords[1] ) + "|" + elsB;
                if (visiblePatchesByID.count(IDtoDisable)) visiblePatchesByID[IDtoDisable] = 0;
                if (allPatchesByID.count(IDtoDisable)) allPatchesByID[IDtoDisable].children = children;
                for (auto childID : children){
                    if (visiblePatchesByID.count(childID)) if (visiblePatchesByID[childID] != 0) visiblePatchesByID[childID] = 1;
                    if (!allPatchesByID.count(childID) && checkInBound(coords)) {
                        Patch p = Patch(childID, each.second[i-1]);
                        p.mapMgr = mapMgr;
                        p.coords = toCoords(childID);
                        p.edgeLength = edgeLength;
                        p.east = cts( p.coords[0] );
                        p.north = cts( p.coords[1] );
                        p.els = els;
                        string fileOrtho = "fdop20_32" + p.east + "_" + p.north + "_rgbi_" + els + ".jpg";
                        string fileHeight = "dgm_E32" + p.east + ".5_N" + p.north + ".5_S"+ els + ".tif";
                        p.orthoPic = localPathOrtho + "/" + els + "/" + fileOrtho;
                        p.heightPic = localPathHeight + "/" + els + "/" + fileHeight;
                        p.localPos = Vec3d( coords[0]-atlasOrigin[0], 0, - (coords[1]-atlasOrigin[1]) ) + Vec3d(edgeLength*0.5,0,edgeLength*0.5); //Vec3d(coordx, 0, coordz);
                        allPatchesByID[childID] = p;
                    }
                }
            }
        }
    }
    for (auto each : highLevel){
        if (visiblePatchesByID.count(each.first)) if (visiblePatchesByID[each.first] < 0) visiblePatchesByID[each.first] = 1;
    }

    for (auto& each: allPatchesByID) {
        string sID = each.first;
        if (!visiblePatchesByID.count(sID)) {
            allPatchesByID[sID].visibleToBe = false;
            if (allPatchesByID[sID].visible) patchQueue.push_back(sID);
        } else {
            if (visiblePatchesByID[sID] == 1) allPatchesByID[sID].visibleToBe = true;
            else { allPatchesByID[sID].visibleToBe = false;}
            patchQueue.push_back(sID);
        }
    }

    handleJobQueue();
    return;
    /*
    if (stop) return;

    layout.steady = !needsShift(layout.innerQuad);
    if (layout.steady) sinceLastMovement++;
    if (shifted) {
        sinceLastMovement = 0;
        resetJobQueue();
    }
    if (shifted) return;

    //cout << defCamPos << endl;
    //if (sinceLastMovement == 60) fillQueue();
    if (sinceLastMovement > 20) handleJobQueue();
    if (debugMode) layout.debugPaint();*/
}

void VRAtlas::setCoordOrigin(double east, double north) { atlasOrigin = Vec2d(east,north); }
void VRAtlas::setBoundary(double minEast, double maxEast, double minNorth, double maxNorth) { bounds = Boundary(minEast, maxEast, minNorth, maxNorth); }
void VRAtlas::setLocalPaths(string ortho, string height) { localPathOrtho = ortho; localPathHeight = height; }
void VRAtlas::setDebug(bool mode) { debugMode = mode; }
void VRAtlas::setScale(float s) { scaling = s; if (atlas) atlas->setScale(Vec3d(s,s,s)); }
void VRAtlas::repaint() { }

Vec3d VRAtlas::getLocalPos(double east, double north) {
    Vec3d pos = Vec3d(east - atlasOrigin[0],0, - (north - atlasOrigin[1]) );
    return pos;
}

VRTransformPtr VRAtlas::setup() {
    cout << "VRAtlas::setup" << endl;
    updatePtr = VRUpdateCb::create("atlas update", bind(&VRAtlas::update, this));
    VRScene::getCurrent()->addUpdateFkt(updatePtr);
    //debugQuad = generatePatch("innerQuadOrigin");

    atlas = VRTransform::create("AtlasTransform");
    atlas->setScale(Vec3d(scaling, scaling, scaling));
    atlas->addChild(debugQuad);

    LODMax = 3;

    return atlas;
}
