#ifndef VRTRAFFICLIGHTS_H_INCLUDED
#define VRTRAFFICLIGHTS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/objects/VRTransform.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRTrafficLight : public VRTransform {
    private:
        VRRoadPtr road;
        VRTrafficLightsPtr system;

    public:
        VRTrafficLight(VRRoadPtr road, VRTrafficLightsPtr system);
        ~VRTrafficLight();

        static VRTrafficLightPtr create(VRRoadPtr road, VRTrafficLightsPtr system);
};

class VRTrafficLights {
    private:
        vector<VRTrafficLightPtr> lights;
        VRRoadIntersectionPtr intersection;

    public:
        VRTrafficLights(VRRoadIntersectionPtr intersection);
        ~VRTrafficLights();

        static VRTrafficLightsPtr create(VRRoadIntersectionPtr intersection);
};

OSG_END_NAMESPACE;

#endif // VRTRAFFICLIGHTS_H_INCLUDED
