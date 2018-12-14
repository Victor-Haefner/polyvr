#include "VRTrafficSigns.h"
#include "VRRoadNetwork.h"
#include "VRRoadIntersection.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>

#include "core/objects/geometry/OSGGeometry.h"

#include "../VRWorldGenerator.h"
#include "../terrain/VRTerrain.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/toString.h"
#include "core/math/path.h"
#include "core/objects/material/VRTextureMosaic.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/Semantics/Reasoning/VREntity.h"

#include "core/utils/system/VRSystem.h"
#include <boost/filesystem.hpp>

using namespace OSG;

VRTrafficSigns::VRTrafficSigns() : VRRoadBase("TrafficSigns") {}
VRTrafficSigns::~VRTrafficSigns() {}

VRTrafficSignsPtr VRTrafficSigns::create() {
    cout << "VRTrafficSigns::create" << endl;
    auto r = VRTrafficSignsPtr( new VRTrafficSigns() );
    r->setMatrix("CN");
    r->loadTextures();
    r->setupBaseSign();
    r->setPersistency(0);
    return r;
}

void VRTrafficSigns::setupBaseSign(){
    trafficSignsGeo = VRGeometry::create("trafficSignsTop");
    trafficSignsGeoPoles = VRGeometry::create("trafficSignsPoles");
    this->addChild(trafficSignsGeo);
    this->addChild(trafficSignsGeoPoles);

    baseGeoSign = VRGeometry::create("trafficSignTop");
    baseGeoSign->setPrimitive("Plane 0.6 0.6 1 1");
    baseGeoSign->translate(Vec3d(0,2.3,0));

    baseGeoPole = VRGeometry::create("trafficSignPole");
    baseGeoPole->setPrimitive("Plane 0.05 2.7 1 1");
    baseGeoPole->translate(Vec3d(0,1.0,0));

    baseMaterial = VRMaterial::create("trafficSignMat");
    baseMaterial->setTexture( megaTex );
    baseMaterial->enableTransparency();
    baseMaterial->setLit(true);
    //baseMaterial->setShaderParameter<float>("tex", 0);
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/TrafficSigns/";
    vScript = resDir + "TrafficSigns.vp";
    fScript = resDir + "TrafficSigns.fp";
    dfScript = resDir + "TrafficSigns.dfp";
    reloadShader();

    //baseMaterialPole = VRMaterial::create("trafficSignMatPole");
    baseMaterialPole = VRMaterial::get("trafficSignMatPole");
    baseMaterialPole->setDiffuse(Color3f(0.3,0.3,0.3));
    baseMaterialPole->setLit(true);

    trafficSignsGeo->setMaterial(baseMaterial);
    trafficSignsGeoPoles->setMaterial(baseMaterialPole);
}

void VRTrafficSigns::reloadShader() {
    baseMaterial->readVertexShader(vScript);
    baseMaterial->readFragmentShader(fScript);
    baseMaterial->readFragmentShader(dfScript, true);
    baseMaterial->updateDeferredShader();
}

Vec2i VRTrafficSigns::getVecID(string type) {
    int divider1 = type.find(":")+1;
    int divider2 = type.find(":",divider1+1)+1;
    string subClass = type.substr(divider1,divider2-divider1-1);
    string subName = type.substr(divider2);
    if (!matrix.count(subClass)) return Vec2i(4,3);
    if (!matrix[subClass].count(subName)) return Vec2i(4,3);
    return matrix[subClass][subName];
}

void VRTrafficSigns::addSign(string type, PosePtr pose) {
    VRGeometryPtr sign = static_pointer_cast<VRGeometry>( baseGeoSign->duplicate() );
    VRGeometryPtr pole = static_pointer_cast<VRGeometry>( baseGeoPole->duplicate() );
    sign->makeUnique();
    sign->setMaterial(baseMaterial);
    pole->setMaterial(baseMaterialPole);
    auto signID = getVecID(type);
    auto coords = megaTex->getChunkUVasVector(signID);
    GeoVectorPropertyRecPtr tc = GeoVec2fProperty::create();
    //cout << toString(coords) << endl;
    for (auto vec : coords) tc->addValue(vec);
    sign->setTexCoords(tc);
    auto pose1 = sign->getPose();
    pose1->setPos(pose1->pos() + pose->pos() - pose->dir()*0.01);
    pose1->setDir(pose->dir());
    pose1->setUp(pose->up());
    auto pose2 = pole->getPose();
    pose2->setPos(pose2->pos() + pose->pos());
    pose2->setDir(pose->dir());
    pose2->setUp(pose->up());
    trafficSignsGeo->merge(sign,pose1);
    trafficSignsGeoPoles->merge(pole,pose2);
}

void VRTrafficSigns::setMegaTexture(VRTextureMosaicPtr megaTex){
    this->megaTex = megaTex;
}

PosePtr VRTrafficSigns::getPosition(int ID) {
    PosePtr res;
    return res;
}

VRTextureMosaicPtr VRTrafficSigns::getTextureMosaic() {
    return megaTex;
}

string VRTrafficSigns::getName(Vec2i ID) {
    if ( signNameByID.count(ID) ) return signNameByID[ID];
    return "No sign with this ID";
}

string VRTrafficSigns::getOSMTag(Vec2i ID) {
    if ( country == "CN" && signNameByID.count(ID) ) return country+":"+types[ID[0]]+":"+signNameByID[ID];
    return "No sign with this ID";
}

void VRTrafficSigns::loadTextures(){
    string path = "world/assets/roadsigns/";
    if (!exists(path)) path = "../world/assets/roadsigns/";
    megaTex = VRTextureMosaic::create();
    auto seedTex = VRTexture::create();
    string sTexFile =  path+"emptyPixel.png";
    seedTex->read(sTexFile);
	megaTex->add(seedTex, Vec2i(0,0), Vec2i(0,0));
	//cout << path << endl;

	if (country == "CN") {
	    /*
        auto getSignName = [&](string input){
            string res = "";
            if (country == "CN") {
                int divider1 = type.find("__")+1;
                int divider2 = type.find(".",divider1+1)+1;
                //string res = type.substr(divider1,divider2-divider1-1);
                string res = type.substr(divider1);
                if (res.substr(0,1) == "0") res = res.substr(1);
            }
            return res;
        };
        string folder = "world/assets/roadsigns/China";
        int nType = 0;
        for (auto f : openFolder(folder)) {
            string type = f;
            string subFolder = folder+"/"+f;
            int nSign = 0;
            typesByID[nType] = f;
            cout << subFolder << endl;
            for (auto pic : openFolder(subFolder)) {
                if (nSign > maxSignsPerRow) continue;
                string signName = getSignName(pic);
                cout << nSign << signName << " ";
                nSign ++;
                continue;
                matrix[type][signName] = Vec2i(nType, nSign);
                auto singleTex = VRTexture::create();
                singleTex->read(subFolder+"/"+pic);
                megaTex->add( singleTex, Vec2i(nType*100, nSign*100), Vec2i(nType,nSign) );
                nSign++;
            }
            nType++;
            cout << endl;
        }*/

        string fileBaseName = "China_road_sign__";
        path = path+"China/";
        if (!exists(path)) return;
        int nType = 0;
        for (auto type : types) {
            int nSign = 0;
            for (auto signName : allFileNames[nType]) {
                if (nSign > maxSignsPerRow) continue;
                string filePath = path+type+"/"+fileBaseName+signName;
                auto singleTex = VRTexture::create();
                singleTex->read(filePath);
                megaTex->add( singleTex, Vec2i(nType*100,nSign*100), Vec2i(nType,nSign) );
                nSign++;
            }
            nType++;
        }
        megaTex->write(path+"trafficSignMegaTex.png");
    }
    if (country == "DE") {
        //example TODO
        path = path+ "Germany/";
    }
}

void VRTrafficSigns::setMatrix(string country){
    this->country  = country;
    if ( country == "CN" ) {
        matrix.clear();
        types = {"Additional", "Indicative", "Informational", "Prohibitory", "Tourist", "Vehicle-mounted", "Warning"};
        vector<vector<string>> allSigns = {
            {"1a", "1b", "2", "3", "4", "5", "6", "7a", "7b", "7c", "7d", "7e", "7f", "7g", "7h", "8", "9", "10", "11", "12", "13", "15", "16", "17", "18", "19", "20", "22"},
            {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12b", "13", "14", "15", "1650", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "28", "29", "30", "31", "32", "33", "34", "35a", "35b", "35c", "35d", "36"},
            {"1a", "1b", "2a", "2b", "3a", "3b", "4a", "4b", "4c", "4d", "5a", "5b", "5c", "5d", "5e", "6", "7", "8a", "8b", "9a", "9b", "10", "11", "12", "13", "14a", "14d", "15", "16", "17b", "18", "19", "20a", "20b", "21a", "21b", "21c", "22a", "22b", "23", "24", "25", "26", "27a", "27b", "28", "29a", "29b", "30a", "30b", "30c", "31", "32a", "32b", "33", "34", "35a", "35b", "36a", "36b", "37a2", "37a3", "37c", "38", "39", "40", "42a1", "42a2", "42b1", "42b2", "43a", "43b", "44a", "44b", "45", "46a", "46b", "47a", "47b", "47c", "48a", "48b", "48c", "48d", "49a", "49b", "50a", "50b", "50c", "50d", "50e", "50f", "50g", "50h", "51a", "51b", "51c", "51d", "51e", "51f", "51g", "51h", "52a", "52b", "52c", "52d", "53-2", "53a", "53b", "54-2a", "54-2b", "54-2c", "54a", "54b", "54c", "55", "56-2", "56a", "56b", "57", "58-2", "58", "59", "60", "61a", "61b", "61c", "61d", "61e", "62a", "62b", "63", "64a", "64b", "64c", "65a", "65b", "66a", "66b", "66c", "66d", "67a", "67b", "67c", "67d", "68a", "68b", "69", "70", "71", "72a", "72b", "72c", "72d", "72e", "72f", "72g", "72h", "73a", "73b", "73c", "74a", "74b", "74c", "75a", "75b", "76a", "76b", "76c", "76d", "77a", "77b", "77c", "77d", "78a", "78b", "78c", "78d", "78e", "79"},
            {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "22a", "23", "23a", "24", "25", "26", "27", "28", "29", "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44", "45", "46", "47", "48"},
            {"1", "2a", "2b", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17"},
            {"342a", "342b", "342c1", "342c2", "342d1", "342d2", "342e", "342f", "342g", "343"},
            {"1a", "1b", "1c", "1d", "1e", "1f", "1g", "1h", "1i", "1j", "2a", "2b", "3a", "3b", "4b", "5a", "5b", "6", "7a", "7b", "7c", "8", "9", "10", "11", "12", "13", "13-2", "14", "15a", "15b", "16", "17", "18a", "18b", "19a", "19b", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "32", "33", "34", "35", "36a", "36b", "36c", "37", "38", "39", "40", "41", "42", "43a", "43b", "44a", "44b", "45a1", "45a2", "45b1_1km", "45b1_2km", "45b1_500m", "45b2_1km", "45b2_2km", "45b2_500m", "45c1", "45c2", "46a", "46b", "46c", "46d", "47", "Stop_Ahead"},
        };
        allFileNames = {
            {"01a.jpg", "01b.jpg", "02.jpg", "03.jpg", "04.jpg", "05.jpg", "06.jpg", "07a.jpg", "07b.jpg", "07c.jpg", "07d.jpg", "07e.jpg", "07f.jpg", "07g.jpg", "07h.jpg", "08.jpg", "09.jpg", "10.jpg", "11.jpg", "12.jpg", "13.jpg", "15.jpg", "16.jpg", "17.jpg", "18.jpg", "19.jpg", "20.jpg", "22.jpg"},
            {"01.gif", "02.gif", "03.gif", "04.gif", "05.gif", "06.gif", "07.gif", "08.gif", "09.gif", "10.gif", "11.gif", "12b.png", "13.png", "14.gif", "15.gif", "1650.png", "17.gif", "18.gif", "19.png", "20.gif", "21.gif", "22.gif", "23.gif", "24.gif", "25.gif", "26.gif", "28.gif", "29.gif", "30.gif", "31.gif", "32.gif", "33.gif", "34.gif", "35a.gif", "35b.gif", "35c.gif", "35d.gif", "36.gif"},
            {"01a.jpg", "01b.jpg", "02a.jpg", "02b.jpg", "03a.jpg", "03b.jpg", "04a.jpg", "04b.jpg", "04c.jpg", "04d.jpg", "05a.jpg", "05b.jpg", "05c.jpg", "05d.jpg", "05e.jpg", "06.jpg", "07.jpg", "08a.jpg", "08b.jpg", "09a.jpg", "09b.jpg", "10.jpg", "11.jpg", "12.jpg", "13.jpg", "14a.jpg", "14d.jpg", "15.jpg", "16.jpg", "17b.jpg", "18.jpg", "19.jpg", "20a.jpg", "20b.jpg", "21a.jpg", "21b.jpg", "21c.jpg", "22a.jpg", "22b.jpg", "23.jpg", "24.jpg", "25.jpg", "26.jpg", "27a.jpg", "27b.jpg", "28.jpg", "29a.jpg", "29b.jpg", "30a.jpg", "30b.jpg", "30c.jpg", "31.jpg", "32a.jpg", "32b.jpg", "33.jpg", "34.jpg", "35a.jpg", "35b.jpg", "36a.jpg", "36b.jpg", "37a2.jpg", "37a3.jpg", "37c.jpg", "38.jpg", "39.jpg", "40.jpg", "42a1.jpg", "42a2.jpg", "42b1.jpg", "42b2.jpg", "43a.jpg", "43b.jpg", "44a.jpg", "44b.jpg", "45.jpg", "46a.jpg", "46b.jpg", "47a.jpg", "47b.jpg", "47c.jpg", "48a.jpg", "48b.jpg", "48c.jpg", "48d.jpg", "49a.jpg", "49b.jpg", "50a.jpg", "50b.jpg", "50c.jpg", "50d.jpg", "50e.jpg", "50f.jpg", "50g.jpg", "50h.jpg", "51a.jpg", "51b.jpg", "51c.jpg", "51d.jpg", "51e.jpg", "51f.jpg", "51g.jpg", "51h.jpg", "52a.jpg", "52b.jpg", "52c.jpg", "52d.jpg", "53-2.jpg", "53a.jpg", "53b.jpg", "54-2a.jpg", "54-2b.jpg", "54-2c.jpg", "54a.jpg", "54b.jpg", "54c.jpg", "55.jpg", "56-2.jpg", "56a.jpg", "56b.jpg", "57.jpg", "58-2.jpg", "58.jpg", "59.jpg", "60.jpg", "61a.jpg", "61b.jpg", "61c.jpg", "61d.jpg", "61e.jpg", "62a.jpg", "62b.jpg", "63.jpg", "64a.jpg", "64b.jpg", "64c.jpg", "65a.jpg", "65b.jpg", "66a.jpg", "66b.jpg", "66c.jpg", "66d.jpg", "67a.jpg", "67b.jpg", "67c.jpg", "67d.jpg", "68a.jpg", "68b.jpg", "69.jpg", "70.jpg", "71.jpg", "72a.jpg", "72b.jpg", "72c.jpg", "72d.jpg", "72e.jpg", "72f.jpg", "72g.jpg", "72h.jpg", "73a.jpg", "73b.jpg", "73c.jpg", "74a.jpg", "74b.jpg", "74c.jpg", "75a.jpg", "75b.jpg", "76a.jpg", "76b.jpg", "76c.jpg", "76d.jpg", "77a.jpg", "77b.jpg", "77c.jpg", "77d.jpg", "78a.jpg", "78b.jpg", "78c.jpg", "78d.jpg", "78e.jpg", "79.jpg"},
            {"01.gif", "02.gif", "03.gif", "04.gif", "05.gif", "06.gif", "07.gif", "08.gif", "09.gif", "10.gif", "11.gif", "12.gif", "13.gif", "14.gif", "15.gif", "16.gif", "17.gif", "18.gif", "19.gif", "20.gif", "21.gif", "22.gif", "22a.gif", "23.gif", "23a.gif", "24.gif", "25.gif", "26.gif", "27.gif", "28.gif", "29.png", "30.png", "31.gif", "32.gif", "33.gif", "34.gif", "35.gif", "36.gif", "37.gif", "38.gif", "39.gif", "40.gif", "41.gif", "42.gif", "43.gif", "44.gif", "45.gif", "46.gif", "47.gif", "48.gif"},
            {"01.jpg", "02a.jpg", "02b.jpg", "03.jpg", "04.jpg", "05.jpg", "06.jpg", "07.jpg", "08.jpg", "09.jpg", "10.jpg", "11.jpg", "12.jpg", "13.jpg", "14.jpg", "15.jpg", "16.jpg", "17.jpg"},
            {"342a.jpg", "342b.jpg", "342c1.jpg", "342c2.jpg", "342d1.jpg", "342d2.jpg", "342e.jpg", "342f.jpg", "342g.jpg", "343.jpg"},
            {"01a.png", "01b.png", "01c.gif", "01d.gif", "01e.gif", "01f.gif", "01g.png", "01h.png", "01i.png", "01j.png", "02a.gif", "02b.gif", "03a.png", "03b.png", "04b.gif", "05a.gif", "05b.gif", "06.gif", "07a.png", "07b.png", "07c.png", "08.gif", "09.png", "10.png", "11.gif", "12.gif", "13.gif", "13-2.png", "14.png", "15a.gif", "15b.gif", "16.gif", "17.gif", "18a.gif", "18b.gif", "19a.gif", "19b.gif", "20.gif", "21.gif", "22.gif", "23.gif", "24.gif", "25.gif", "26.gif", "27.gif", "28.gif", "29.gif", "30.gif", "32.gif", "33.gif", "34.gif", "35.gif", "36a.gif", "36b.gif", "36c.gif", "37.gif", "38.gif", "39.gif", "40.gif", "41.gif", "42.gif", "43a.gif", "43b.gif", "44a.gif", "44b.gif", "45a1.gif", "45a2.gif", "45b1_1km.gif", "45b1_2km.gif", "45b1_500m.gif", "45b2_1km.gif", "45b2_2km.gif", "45b2_500m.gif", "45c1.gif", "45c2.gif", "46a.gif", "46b.gif", "46c.gif", "46d.gif", "47.gif", "Stop_Ahead.png"},
        };
        int nType = 0;
        for (auto type : types) {
            int i = 0;
            for (auto sign : allSigns[nType]) {
                matrix[types[nType]][sign] = Vec2i(nType,i);
                signNameByID[Vec2i(nType,i)] = sign;
                i++;
            }
            nType++;
        }
    }
    //matrix[signType][OSMSignTag]
}

VRGeometryPtr VRTrafficSigns::getGeometry() {
    if (selfPtr) return selfPtr;
    return 0;
}

