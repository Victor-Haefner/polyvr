#include "VRTrafficSigns.h"
#include "VRRoadNetwork.h"
#include "VRRoadIntersection.h"
#include "../VRWorldGenerator.h"
#include "../terrain/VRTerrain.h"
#include "core/utils/toString.h"
#include "core/math/path.h"
#include "core/objects/geometry/VRStroke.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/Semantics/Reasoning/VREntity.h"

using namespace OSG;

VRTrafficSigns::VRTrafficSigns() : VRRoadBase("TrafficSigns") {}
VRTrafficSigns::~VRTrafficSigns() {}

VRTrafficSignsPtr VRTrafficSigns::create() {
    auto r = VRTrafficSignsPtr( new VRTrafficSigns() );
    return r;
}

void VRTrafficSigns::setMegaTexture(VRTextureMosaicPtr megaTex){
    this->megaTex = megaTex;
}

PosePtr VRTrafficSigns::getPosition(int ID) {
    PosePtr res;
    return res;
}

void VRTrafficSigns::setMatrix(string country){
    map<string,map<string,Vec2i>> matrix;
    this->country  = country;
    if ( country == "CN" ) {
        vector<string> types = {"Additional", "Indicative", "Informational", "Prohibitory", "Tourist", "Vehicle-mounted", "Warning"};
        vector<vector<string>> allSigns = {
            {"1a", "1b", "2", "3", "4", "5", "6", "7a", "7b", "7c", "7d", "7e", "7f", "7g", "7h", "8", "9", "10", "11", "12", "13", "15", "16", "17", "18", "19", "20", "22"},
            {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12b", "13", "14", "15", "1650", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "28", "29", "30", "31", "32", "33", "34", "35a", "35b", "35c", "35d", "36"},
            {"1a", "1b", "2a", "2b", "3a", "3b", "4a", "4b", "4c", "4d", "5a", "5b", "5c", "5d", "5e", "6", "7", "8a", "8b", "9a", "9b", "10", "11", "12", "13", "14a", "14d", "15", "16", "17b", "18", "19", "20a", "20b", "21a", "21b", "21c", "22a", "22b", "23", "24", "25", "26", "27a", "27b", "28", "29a", "29b", "30a", "30b", "30c", "31", "32a", "32b", "33", "34", "35a", "35b", "36a", "36b", "37a2", "37a3", "37c", "38", "39", "40", "42a1", "42a2", "42b1", "42b2", "43a", "43b", "44a", "44b", "45", "46a", "46b", "47a", "47b", "47c", "48a", "48b", "48c", "48d", "49a", "49b", "50a", "50b", "50c", "50d", "50e", "50f", "50g", "50h", "51a", "51b", "51c", "51d", "51e", "51f", "51g", "51h", "52a", "52b", "52c", "52d", "53-2", "53a", "53b", "54-2a", "54-2b", "54-2c", "54a", "54b", "54c", "55", "56-2", "56a", "56b", "57", "58-2", "58", "59", "60", "61a", "61b", "61c", "61d", "61e", "62a", "62b", "63", "64a", "64b", "64c", "65a", "65b", "66a", "66b", "66c", "66d", "67a", "67b", "67c", "67d", "68a", "68b", "69", "70", "71", "72a", "72b", "72c", "72d", "72e", "72f", "72g", "72h", "73a", "73b", "73c", "74a", "74b", "74c", "75a", "75b", "76a", "76b", "76c", "76d", "77a", "77b", "77c", "77d", "78a", "78b", "78c", "78d", "78e", "79"},
            {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "22a", "23", "23a", "24", "25", "26", "27", "28", "29", "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44", "45", "46", "47", "48"},
            {"1", "2a", "2b", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17"},
            {"342a", "342b", "342c1", "342c2", "342d1", "342d2", "342e", "342f", "342g", "343"},
            {"1a", "1b", "1c", "1d", "1e", "1f", "1g", "1h", "1i", "1j", "2a", "2b", "3a", "3b", "4b", "5a", "5b", "6", "7a", "7b", "7c", "8", "9", "10", "11", "12", "13-2", "13", "14", "15a", "15b", "16", "17", "18a", "18b", "19a", "19b", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "32", "33", "34", "35", "36a", "36b", "36c", "37", "38", "39", "40", "41", "42", "43a", "43b", "44a", "44b", "45a1", "45a2", "45b1_1km", "45b1_2km", "45b1_500m", "45b2_1km", "45b2_2km", "45b2_500m", "45c1", "45c2", "46a", "46b", "46c", "46d", "47", "Stop_Ahead"},
        };
        int nType = 0;
        for (auto type : types) {
            int i = 0;
            for (auto sign : allSigns[nType]) {
                matrix[types[nType]][sign] = Vec2i(nType,i);
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

