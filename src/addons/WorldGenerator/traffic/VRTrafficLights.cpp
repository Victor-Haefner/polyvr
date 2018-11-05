#include "VRTrafficLights.h"
#include "core/scene/VRScene.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/math/pose.h"

#include <GL/glut.h>

using namespace OSG;

VRTrafficLight::VRTrafficLight(VREntityPtr lane, VRTrafficLightsPtr system) : VRTransform("trafficLight"), lane(lane), system(system) {}
VRTrafficLight::~VRTrafficLight() {}

VRTrafficLightPtr VRTrafficLight::create(VREntityPtr lane, VRTrafficLightsPtr system, PosePtr p) {
    auto l = VRTrafficLightPtr(new VRTrafficLight(lane, system));
    l->setPose(p);
    system->addTrafficLight(l);
    return l;
}

void VRTrafficLight::setupBulbs(VRGeometryPtr red, VRGeometryPtr orange, VRGeometryPtr green) {
    auto redOff = VRMaterial::get("redOff");
    auto orangeOff = VRMaterial::get("orangeOff");
    auto greenOff = VRMaterial::get("greenOff");

    if (red) red->setMaterial(redOff);
    if (orange) orange->setMaterial(orangeOff);
    if (green) green->setMaterial(greenOff);

    this->red = red;
    this->orange = orange;
    this->green = green;
}

void VRTrafficLight::setState(string s) {
    auto sys = system.lock();
    if (s[0] != state[0] && red) { if (s[0] == '1') red->setMaterial(sys->redOn); else red->setMaterial(sys->redOff); }
    if (s[1] != state[1] && orange) { if (s[1] == '1') orange->setMaterial(sys->orangeOn); else orange->setMaterial(sys->orangeOff); }
    if (s[2] != state[2] && green) { if (s[2] == '1') green->setMaterial(sys->greenOn); else green->setMaterial(sys->greenOff); }
    state = s;
}

string VRTrafficLight::getState() { return state; }
void VRTrafficLight::setUsingAsset(bool check) { isUsingAsset = check; }
bool VRTrafficLight::getUsingAsset() { return isUsingAsset; }
VRTrafficLights::VRTrafficLights() {
    redOff = VRMaterial::get("redOff");
    orangeOff = VRMaterial::get("orangeOff");
    greenOff = VRMaterial::get("greenOff");
    redOn = VRMaterial::get("redOn");
    orangeOn = VRMaterial::get("orangeOn");
    greenOn = VRMaterial::get("greenOn");

    static bool doInitColors = true;
    if (doInitColors) {
        doInitColors = false;
        float C = 0.3;
        redOff->setDiffuse(Color3f(C,0,0));
        orangeOff->setDiffuse(Color3f(C,C,0));
        greenOff->setDiffuse(Color3f(0,C,0));
        redOff->setLit(true);
        orangeOff->setLit(true);
        greenOff->setLit(true);

        redOn->setDiffuse(Color3f(1.0,0,0));
        orangeOn->setDiffuse(Color3f(1.0,1.0,0));
        greenOn->setDiffuse(Color3f(0,1.0,0));
        redOn->setLit(false);
        orangeOn->setLit(false);
        greenOn->setLit(false);
    }
}

VRTrafficLights::~VRTrafficLights() {}
VRTrafficLightsPtr VRTrafficLights::create() { return VRTrafficLightsPtr(new VRTrafficLights()); }

void VRTrafficLights::addTrafficLight(VRTrafficLightPtr light) {
    auto opposite = [&](VRTrafficLightPtr l1, VRTrafficLightPtr l2) { // TODO: this is pretty crappy..
        /*auto lightE1 = l1->getEntity();
        auto lightE2 = l2->getEntity();
        auto laneE1 = l1->lane;
        auto laneE2 = l2->lane;*/

        auto p1 = l1->getPose();
        auto p2 = l2->getPose();

        Vec3d d1 = p1->up();
        Vec3d d2 = p2->up();

        if (light->getUsingAsset()) {
            p1 = l1->getWorldPose();
            p2 = l2->getWorldPose();
            d1 = p1->up();
            d2 = p2->up();
        }

        d1.normalize();
        d2.normalize();

        float a = abs(d1.dot(d2));
        if (a > 0.5) return true;

        //return true;
        return false;
    };

    for (auto& group : lights) {
        int offset = group.first;
        for (auto& l : group.second) {
            if (!opposite(l, light)) continue;
            group.second.push_back(light);
            return;
        }
    }
    // new group -> recompute keys
    lights[-1].push_back(light);
    map<int, vector<VRTrafficLightPtr> > newLights;
    for (auto& group : lights) {
        int i = newLights.size();
        int grp = i*60.0/lights.size();
        newLights[grp] = group.second;
    }
    lights = newLights;
}

vector<VRTrafficLightPtr> VRTrafficLights::getLights() {
    vector<VRTrafficLightPtr> res;
    for (auto& group : lights) {
        for (auto& l : group.second) {
            res.push_back(l);
        }
    }
    return res;
}

map<int, vector<VRTrafficLightPtr>> VRTrafficLights::getMap() {
    return lights;
}

void VRTrafficLights::update() { // TODO, use time instead of counter!
    static int t = 0;
    static int t1 = 0; t1++;
    if (t1 > 120) { t1 = 0; t++; }

    auto orangeBlinking = [&](VRTrafficLightPtr& l) {
        if (t%6 < 3) l->setState("010");
        if (t%6 >= 3) l->setState("000");
    };

    auto mainCycle = [&](VRTrafficLightPtr& l, int offset) {
        int a = (t+offset)%60;
        if (a ==  0) l->setState("100"); //red|orange|green
        if (a == 30) l->setState("110");
        if (a == 34) l->setState("001");
        if (a == 56) l->setState("010");
    };

    auto allOn = [&](VRTrafficLightPtr& l) {
        l->setState("111");
    };
    auto allOff = [&](VRTrafficLightPtr& l) {
        l->setState("000");
    };

    for (auto& group : lights) {
        int offset = group.first;
        for (auto& l : group.second) {
            //allOff(l);
            //allOn(l);
            //orangeBlinking(l);
            mainCycle(l, offset);
        }
    }


    //cout << "switched traffic light " << toString(t) << endl;
}

