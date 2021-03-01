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
    if (megaTex) {
        auto coords = megaTex->getChunkUVasVector(signID);
        GeoVectorPropertyRecPtr tc = GeoVec2fProperty::create();
        //cout << toString(coords) << endl;
        for (auto vec : coords) tc->addValue(vec);
        sign->setTexCoords(tc);
    }
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
    if (!exists(path)) { //TODO: create Emtpy Pixel
        return;
    }
    string texPath = path;
    megaTex = VRTextureMosaic::create();
    auto seedTex = VRTexture::create();
    string sTexFile =  path+"emptyPixel.png";
    seedTex->read(sTexFile);
	megaTex->add(seedTex, Vec2i(0,0), Vec2i(0,0));
	//cout << path << endl;

	if (country == "CN") {
        string fileBaseName = "";
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
        megaTex->write(texPath+"trafficSignMegaTex.png");
    }
    if (country == "DE") {
        //example TODO
        path = path+ "Germany/";
    }
}

void VRTrafficSigns::setMatrix(string country){
    this->country  = country;
    string path = "world/assets/roadsigns/";
    if (!exists(path)) path = "../world/assets/roadsigns/";
    if ( country == "CN" ) {
        path = path+"China/";
        matrix.clear();
        //types = {"Additional", "Indicative", "Informational", "Prohibitory", "Tourist", "Vehicle-mounted", "Warning"};

        vector<vector<string>> allSigns;
        for (auto f : openFolder(path)) {
            types.push_back(f);
            //cout << f << endl;
            string typeFolder = path+f;
            vector<string> tt;
            vector<string> oN;
            for (auto n : openFolder(typeFolder)) {
                int fAt = n.length() - 1;
                if ( n.find(".") < n.length() + 1 ) fAt = n.find(".");
                string nOnly = n.substr(0,fAt);
                //cout << nOnly << endl;
                tt.push_back(nOnly);
                oN.push_back(n);
            }
            allSigns.push_back(tt);
            allFileNames.push_back(oN);
        }

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

