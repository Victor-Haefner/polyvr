#include "VRPyCodeCompletion.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"

#include <string.h>
#include <iostream>

using namespace std;

VRPyCodeCompletion::VRPyCodeCompletion() {}
VRPyCodeCompletion::~VRPyCodeCompletion() {}

void VRPyCodeCompletion::initVRModMap() {
    vrModMap.clear();
    auto scene = OSG::VRScene::getCurrent();
    if (scene == 0) return;
    for ( auto mod : scene->getPyVRModules() ) {
        if (mod != "VR") vrModMap["VR"].push_back(mod);
        for ( auto t : scene->getPyVRTypes(mod) ) {
            if (t == "globals") {
                for (auto m : scene->getPyVRMethods(mod,t)) vrModMap[mod].push_back(m);
                continue;
            }

            vrModMap[mod].push_back(t);
            //for (auto m : scene->getPyVRMethods(mod,t)) vrModMap[mod].push_back(m);
        }
    }
    vrModMapInitiated = true;
}

vector<string> VRPyCodeCompletion::getSuggestions(string input) {
    auto startsWith = [](string s, string sw) { return s.substr(0, sw.size()) == sw; };

    string mod = "VR";
    vector<string> res;
	if (input.size() <= 0 || !startsWith(input, "VR.")) return res;

    auto psplit = splitString(input, '.');
    if (input[input.size()-1] == '.') psplit.push_back("");
    if (psplit.size() > 1) input = psplit[psplit.size()-1];
    if (psplit.size() > 2) mod = psplit[psplit.size()-2];

	if (!vrModMapInitiated) initVRModMap();
    if (!vrModMap.count(mod)) return res;
    for (auto d : vrModMap[mod]) {
        if (startsWith(d, input)) res.push_back(d);
    }

    return res;
}


