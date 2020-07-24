#ifndef VRLIGHT_H_INCLUDED
#define VRLIGHT_H_INCLUDED

#include "core/objects/object/VRObject.h"
#include "core/math/boundingbox.h"
#include <OpenSG/OSGSField.h>
#include <OpenSG/OSGColor.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class Light; OSG_GEN_CONTAINERPTR(Light);
class VRLightBeacon;
#ifndef OSG_OGL_ES2
class VRShadowEngine; OSG_GEN_CONTAINERPTR(VRShadowEngine);
class SimpleShadowMapEngine; OSG_GEN_CONTAINERPTR(SimpleShadowMapEngine);
class ShaderShadowMapEngine; OSG_GEN_CONTAINERPTR(ShaderShadowMapEngine);
class TrapezoidalShadowMapEngine; OSG_GEN_CONTAINERPTR(TrapezoidalShadowMapEngine);
#endif

class VRLight : public VRObject {
    protected:
        OSGCorePtr d_light;
        OSGCorePtr p_light;
        OSGCorePtr s_light;
        OSGCorePtr ph_light;
        VRLightBeaconWeakPtr beacon;
#ifndef OSG_OGL_ES2
        SimpleShadowMapEngineRefPtr ssme;
        ShaderShadowMapEngineRefPtr gsme;
        TrapezoidalShadowMapEngineRefPtr ptsme;
        TrapezoidalShadowMapEngineRefPtr stsme;
#endif

        string lightType = "point";
        string beacon_name;
        string photometricMapPath;
        VRTexturePtr photometricMap = 0;
        Boundingbox shadowVolume;
        int shadowMapRes = 2048;
        Color4f lightDiffuse;
        Color4f lightAmbient;
        Color4f lightSpecular;
        Color4f shadowColor;
        Vec2d shadowNearFar;
        Vec3d attenuation; // C L Q
        bool shadows = false;
        bool on = true;
        bool deferred = false;

        VRUpdateCbPtr setupAfterCb;

        VRObjectPtr copy(vector<VRObjectPtr> children);

        void setup(VRStorageContextPtr context);
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
        void toggleShadows(bool b);

        void setType(string type);
        void setDeferred(bool b);
        void reloadDeferredSystem();

        void setDiffuse(Color4f c);
        void setAmbient(Color4f c);
        void setSpecular(Color4f c);
        Color4f getDiffuse();
        Color4f getAmbient();
        Color4f getSpecular();

        void setShadowParams(bool b, int res, Color4f c, Vec2d nf = Vec2d());
        void setShadows(bool b);
        bool getShadows();
        void setShadowColor(Color4f c);
        void setShadowNearFar(Vec2d nf);
        void setShadowVolume(Boundingbox b);
        void setShadowMapRes(int t);
        Color4f getShadowColor();
        int getShadowMapRes();
        Boundingbox getShadowVolume();

        VRLightBeaconPtr addBeacon();
        VRLightBeaconPtr getBeacon();
        void setBeacon(VRLightBeaconPtr b);

        void setAttenuation(Vec3d a);
        Vec3d getAttenuation();

        void setPointlight();
        void setSpotlight();
        void setDirectionallight();
        void setPhotometriclight();

        void setPhotometricMap(VRTexturePtr tex);
        void loadPhotometricMap(string path);
        VRTexturePtr getPhotometricMap(bool forVisual = false);

        LightMTRecPtr getLightCore();
        string getLightType();

        static vector<string> getTypes();
        static vector<string> getShadowMapResolutions();
        static vector<string> getTypeParameter(string type);
};

OSG_END_NAMESPACE;

#endif // VRLIGHT_H_INCLUDED
