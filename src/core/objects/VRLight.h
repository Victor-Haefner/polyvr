#ifndef VRLIGHT_H_INCLUDED
#define VRLIGHT_H_INCLUDED

#include <OpenSG/OSGNode.h>

#include "core/objects/object/VRObject.h"
#include <OpenSG/OSGSField.h>
#include <OpenSG/OSGColor.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class Light; OSG_GEN_CONTAINERPTR(Light);
class SimpleShadowMapEngine; OSG_GEN_CONTAINERPTR(SimpleShadowMapEngine);
class ShaderShadowMapEngine; OSG_GEN_CONTAINERPTR(ShaderShadowMapEngine);
class TrapezoidalShadowMapEngine; OSG_GEN_CONTAINERPTR(TrapezoidalShadowMapEngine);
class VRLightBeacon;

class VRLight : public VRObject {
    protected:
        OSGCorePtr d_light;
        OSGCorePtr p_light;
        OSGCorePtr s_light;
        VRLightBeaconWeakPtr beacon;
        SimpleShadowMapEngineRefPtr ssme;
        ShaderShadowMapEngineRefPtr gsme;
        TrapezoidalShadowMapEngineRefPtr tsme;

        string lightType;
        string shadowType;
        string beacon_name;
        Color4f lightDiffuse;
        Color4f lightAmbient;
        Color4f lightSpecular;
        Color4f shadowColor;
        Vec3f attenuation; // C L Q
        bool shadows = false;
        bool on = true;

        void setup();
        void setup_after();
        void setupShadowEngines();
        void updateDeferredLight();

    public:
        VRLight(string name = "");
        ~VRLight();

        static VRLightPtr create(string name = "None");
        VRLightPtr ptr();

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

        VRLightBeaconWeakPtr getBeacon();
        void setBeacon(VRLightBeaconPtr b);

        void setAttenuation(Vec3f a);
        Vec3f getAttenuation();

        void setPointlight();
        void setSpotlight();
        void setDirectionallight();

        LightMTRecPtr getLightCore();
        string getLightType();

        static vector<string> getTypes();
        static vector<string> getShadowTypes();
        static vector<string> getTypeParameter(string type);
};

OSG_END_NAMESPACE;

#endif // VRLIGHT_H_INCLUDED
