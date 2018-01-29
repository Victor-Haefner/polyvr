#include "VRTrafficLights.h"

using namespace OSG;

VRTrafficLight::VRTrafficLight(VREntityPtr lane, VRTrafficLightsPtr system) : VRTransform("bridge"), lane(lane), system(system) {}
VRTrafficLight::~VRTrafficLight() {}

VRTrafficLightPtr VRTrafficLight::create(VREntityPtr lane, VRTrafficLightsPtr system) { return VRTrafficLightPtr(new VRTrafficLight(lane, system)); }


VRTrafficLights::VRTrafficLights(string group) : group(group) {}
VRTrafficLights::~VRTrafficLights() {}

VRTrafficLightsPtr VRTrafficLights::create(string group) { return VRTrafficLightsPtr(new VRTrafficLights(group)); }
