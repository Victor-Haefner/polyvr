#include "VRTrafficLights.h"

using namespace OSG;

VRTrafficLight::VRTrafficLight(VRRoadPtr road, VRTrafficLightsPtr system) : VRTransform("bridge"), road(road), system(system) {}
VRTrafficLight::~VRTrafficLight() {}

VRTrafficLightPtr VRTrafficLight::create(VRRoadPtr road, VRTrafficLightsPtr system) { return VRTrafficLightPtr(new VRTrafficLight(road, system)); }


VRTrafficLights::VRTrafficLights(VRRoadIntersectionPtr intersection) : intersection(intersection) {}
VRTrafficLights::~VRTrafficLights() {}

VRTrafficLightsPtr VRTrafficLights::create(VRRoadIntersectionPtr intersection) { return VRTrafficLightsPtr(new VRTrafficLights(intersection)); }
