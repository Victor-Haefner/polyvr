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
        VREntityPtr lane;
        VRTrafficLightsWeakPtr system;
        string state = "green";

    public:
        VRTrafficLight(VREntityPtr lane, VRTrafficLightsPtr system);
        ~VRTrafficLight();

        static VRTrafficLightPtr create(VREntityPtr lane, VRTrafficLightsPtr system);
        VRTrafficLightsPtr getSystem();
};

class VRTrafficLights {
    private:
        vector<VRTrafficLightPtr> lights;
        string group;

    public:
        VRTrafficLights(string group);
        ~VRTrafficLights();

        static VRTrafficLightsPtr create(string group);

        void addLight(VRTrafficLightPtr light);
        void setLight(VRTrafficLightPtr light, string state);
};

OSG_END_NAMESPACE;

#endif // VRTRAFFICLIGHTS_H_INCLUDED
