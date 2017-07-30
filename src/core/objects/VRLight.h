#ifndef VRLIGHT_H_INCLUDED
#define VRLIGHT_H_INCLUDED

#include "core/objects/object/VRObject.h"
#include <OpenSG/OSGSField.h>
#include <OpenSG/OSGColor.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class Light; OSG_GEN_CONTAINERPTR(Light);
class VRShadowEngine; OSG_GEN_CONTAINERPTR(VRShadowEngine);
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
        VRShadowEngineRecPtr ssme;
        //SimpleShadowMapEngineRefPtr ssme;
        ShaderShadowMapEngineRefPtr gsme;
        TrapezoidalShadowMapEngineRefPtr ptsme;
        TrapezoidalShadowMapEngineRefPtr stsme;

        string lightType = "point";
        string beacon_name;
        int shadowMapRes = 2048;
        Color4f lightDiffuse;
        Color4f lightAmbient;
        Color4f lightSpecular;
        Color4f shadowColor;
        Vec3d attenuation; // C L Q
        bool shadows = false;
        bool on = true;
        bool deferred = false;

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
        void setDeferred(bool b);

        void setDiffuse(Color4f c);
        Color4f getDiffuse();
        void setAmbient(Color4f c);
        Color4f getAmbient();
        void setSpecular(Color4f c);
        Color4f getSpecular();

        void setShadowParams(bool b, int res, Color4f c);
        void setShadows(bool b);
        bool getShadows();
        void setShadowColor(Color4f c);
        Color4f getShadowColor();
        void setShadowMapRes(int t);
        int getShadowMapRes();

        VRLightBeaconWeakPtr getBeacon();
        void setBeacon(VRLightBeaconPtr b);

        void setAttenuation(Vec3d a);
        Vec3d getAttenuation();

        void setPointlight();
        void setSpotlight();
        void setDirectionallight();

        LightMTRecPtr getLightCore();
        string getLightType();

        static vector<string> getTypes();
        static vector<string> getShadowMapResolutions();
        static vector<string> getTypeParameter(string type);
};

OSG_END_NAMESPACE;

#endif // VRLIGHT_H_INCLUDED
