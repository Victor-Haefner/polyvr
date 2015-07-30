#ifndef VRLIGHT_H_INCLUDED
#define VRLIGHT_H_INCLUDED

#include "core/objects/object/VRObject.h"
#include <OpenSG/OSGFieldContainerFields.h>
#include <OpenSG/OSGColor.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class Light; OSG_GEN_CONTAINERPTR(Light);
class PointLight; OSG_GEN_CONTAINERPTR(PointLight);
class DirectionalLight; OSG_GEN_CONTAINERPTR(DirectionalLight);
class SpotLight; OSG_GEN_CONTAINERPTR(SpotLight);
class SimpleShadowMapEngine; OSG_GEN_CONTAINERPTR(SimpleShadowMapEngine);
class VRLightBeacon;

class VRLight : public VRObject {
    protected:
        DirectionalLightRecPtr d_light;
        PointLightRecPtr p_light;
        SpotLightRecPtr s_light;
        string lightType;
        string shadowType;
        VRLightBeacon* beacon = 0;
        string beacon_name;
        Color4f lightDiffuse;
        Color4f lightAmbient;
        Color4f lightSpecular;
        Color4f shadowColor;
        bool shadows = false;
        bool on = true;
        SimpleShadowMapEngineRefPtr ssme;
        Vec3f attenuation; // C L Q

        void saveContent(xmlpp::Element* e);
        void loadContent(xmlpp::Element* e);

        void update();

    public:
        VRLight(string name = "");
        ~VRLight();

        void setOn(bool b);
        bool isOn();

        void setType(string type);

        void setLightDiffColor(Color4f c);
        Color4f getLightDiffColor();
        void setLightAmbColor(Color4f c);
        Color4f getLightAmbColor();
        void setLightSpecColor(Color4f c);
        Color4f getLightSpecColor();

        void setShadows(bool b);
        bool getShadows();
        void setShadowColor(Color4f c);
        Color4f getShadowColor();
        void setShadowType(string t);
        string getShadowType();

        VRLightBeacon* getBeacon();
        void setBeacon(VRLightBeacon* b);

        void setAttenuation(Vec3f a);
        Vec3f getAttenuation();

        void setPointlight();
        void setSpotlight();
        void setDirectionallight();

        LightRecPtr getLightCore();
        string getLightType();

        static vector<string> getTypes();
        static vector<string> getShadowTypes();
        static vector<string> getTypeParameter(string type);
};

OSG_END_NAMESPACE;

#endif // VRLIGHT_H_INCLUDED
