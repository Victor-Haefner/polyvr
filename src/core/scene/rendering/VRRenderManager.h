#ifndef VRRENDERMANAGER_H_INCLUDED
#define VRRENDERMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGColor.h>
#include "core/objects/VRObjectFwd.h"
#include "core/scene/VRSceneFwd.h"
#include "core/setup/VRSetupFwd.h"
#include "core/utils/VRStorage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDeferredRenderStage;

class VRRenderManager : public VRStorage {
    private:
        bool frustumCulling = true;
        bool occlusionCulling = false;
        bool twoSided = true;

        bool deferredRendering = false;
        bool do_ssao = false;
        bool calib = false;
        bool do_hmdd = false;
        bool do_marker = false;
        bool do_fxaa = false;
        int ssao_kernel = 4;
        int ssao_noise = 4;
        float ssao_radius = 0.02;
        Color4f fogParams = Color4f(0, 0, 100, 0.1);
        Color4f fogColor = Color4f(0.5, 0.5, 0.5, 1);

        bool glMSAA = false;

    protected:
        VRObjectPtr root = 0;

        vector<VRRenderStudioPtr> getRenderings();

    public:
        VRRenderManager();
        ~VRRenderManager();

        void addLight(VRLightPtr l);
        void subLight(int ID);
        void updateLight(VRLightPtr l);
        void setDSCamera(VRCameraPtr cam);

        void setFrustumCulling(bool b);
        void setOcclusionCulling(bool b);
        void setTwoSided(bool b);
        bool getFrustumCulling();
        bool getOcclusionCulling();
        bool getTwoSided();

        void addStage(string name, string parent = "");
        void setStageActive(string name, bool da, bool la);
        void setStageShader(string name, string VPpath, string FPpath, bool doDeferred);
        int addStageBuffer(string name, int pformat, int ptype);
        void setStageParameter(string name, string var, int val);
        void setStageTexture(string name, VRTexturePtr tex, int unit, int mag, int min);

        bool getDefferedShading();
        bool getSSAO();
        bool getHMDD();
        bool getMarker();
        bool getCalib();
        bool getFXAA();

        void setFogParams(Color4f fogParams, Color4f fogColor);
        void setDeferredShading(bool b);
        void setDeferredChannel(int channel);
        void setSSAO(bool b);
        void setSSAOradius(float r);
        void setSSAOkernel(int k);
        void setSSAOnoise(int n);
        void setCalib(bool b);
        void setHMDD(bool b);
        void setMarker(bool b);
        void setFXAA(bool b);

        void update();
        void reloadStageShaders();

        static string getGLVendor();
        static string getGLVersion();
        static int getGLSLVersion();
        static bool hasGeomShader();
        static bool hasTessShader();

        bool getMultisampling();
        void setMultisampling(bool b);
};

OSG_END_NAMESPACE;

#endif // VRRENDERMANAGER_H_INCLUDED
