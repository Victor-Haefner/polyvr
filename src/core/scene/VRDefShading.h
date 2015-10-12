#ifndef VRDEFSHADING_H_INCLUDED
#define VRDEFSHADING_H_INCLUDED

#include <OpenSG/OSGDeferredShadingStage.h>
#include <OpenSG/OSGShaderShadowMapEngine.h>
#include <OpenSG/OSGTrapezoidalShadowMapEngine.h>
#include <OpenSG/OSGImage.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDefShading {
    private:
        enum ShadowTypeE {
            ST_NONE      = 0,
            ST_STANDARD  = 1,
            ST_TRAPEZOID = 2
        };

        enum LightTypeE {
            Directional,
            Point,
            Spot
        };

        struct LightInfo {
            LightTypeE    lightType;
            ShadowTypeE                shadowType;
            LightRecPtr              light;
            //NodeRecPtr               lightN;
            //NodeRecPtr               beaconN;

            ShaderProgramRecPtr      lightVP;
            ShaderProgramRecPtr      lightFP;
            ShaderProgramChunkRecPtr lightSH;
        };

        string dsGBufferVPFile, dsGBufferFPFile;
        string dsAmbientVPFile, dsAmbientFPFile;
        string ssaoAmbientVPFile, ssaoAmbientFPFile;
        string dsDirLightVPFile, dsDirLightFPFile, dsDirLightShadowFPFile;
        string dsPointLightVPFile, dsPointLightFPFile, dsPointLightShadowFPFile;
        string dsSpotLightVPFile, dsSpotLightFPFile, dsSpotLightShadowFPFile;
        string dsUnknownFile = "unknownFile";

        NodeRecPtr                   dsStageN;
        DeferredShadingStageRecPtr   dsStage;
        vector<LightInfo>         lightInfos;
        UInt32                         shadowMapWidth;
        UInt32                         shadowMapHeight;

        ShadowTypeE defaultShadowType;
        int shadowRes;
        float shadowColor;
        bool initiated = false;
        bool enabled = false;
        VRObjectPtr stageObject;
        VRMaterialPtr ssao_mat;

        void init();

    public:
        VRDefShading();
        ~VRDefShading();

        void initDeferredShading(VRObjectPtr o);
        void initSSAO(VRObjectPtr o);
        void setSSAOparams(float radius, int kernel);
        //void initSSAO(VRObjectPtr o, VRMaterialPtr mat);

        void setDefferedShading(bool b);
        bool getDefferedShading();

        void setDSCamera(VRCameraPtr cam);
        void addDSLight(VRLightPtr light);
        void addDSLight(LightRecPtr light, string type, bool shadows = false);
        void subLight(UInt32 lightIdx);
        void setShadow(LightInfo &li);

        const std::string &getLightVPFile(LightTypeE lightType);
        const std::string &getLightFPFile(LightTypeE lightType, ShadowTypeE shadowType);
};

OSG_END_NAMESPACE;

#endif // VRDEFSHADING_H_INCLUDED
