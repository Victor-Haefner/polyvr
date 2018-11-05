#ifndef VRTRAFFICLIGHTS_H_INCLUDED
#define VRTRAFFICLIGHTS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/objects/VRTransform.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRTrafficLight;
class VRTrafficLights;

class VRTrafficLight : public VRTransform {
    public:
        VREntityPtr lane;
        VRTrafficLightsWeakPtr system;
        string state = "000";
        bool isUsingAsset = false;

        VRGeometryPtr red;
        VRGeometryPtr orange;
        VRGeometryPtr green;

    public:
        VRTrafficLight(VREntityPtr lane, VRTrafficLightsPtr system);
        ~VRTrafficLight();

        static VRTrafficLightPtr create(VREntityPtr lane, VRTrafficLightsPtr system, PosePtr p);
        VRTrafficLightsPtr getSystem();

        void setupBulbs(VRGeometryPtr red, VRGeometryPtr orange, VRGeometryPtr green);

        void setState(string s);
        string getState();
        void setUsingAsset(bool check);
        bool getUsingAsset();

        friend class VRTrafficLights;
};

class VRTrafficLights {
    public:
        map<int, vector<VRTrafficLightPtr>> lights;

        VRMaterialPtr redOff;
        VRMaterialPtr orangeOff;
        VRMaterialPtr greenOff;
        VRMaterialPtr redOn;
        VRMaterialPtr orangeOn;
        VRMaterialPtr greenOn;

    public:
        VRTrafficLights();
        ~VRTrafficLights();

        static VRTrafficLightsPtr create();

        void addTrafficLight(VRTrafficLightPtr light);

        vector<VRTrafficLightPtr> getLights();
        map<int, vector<VRTrafficLightPtr>> getMap();

        void update();

        friend class VRTrafficLight;
};

OSG_END_NAMESPACE;

#endif // VRTRAFFICLIGHTS_H_INCLUDED
