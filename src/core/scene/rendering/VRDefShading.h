#ifndef VRDEFSHADING_H_INCLUDED
#define VRDEFSHADING_H_INCLUDED

#include <OpenSG/OSGDeferredShadingStage.h>
#include <OpenSG/OSGShaderShadowMapEngine.h>
#include <OpenSG/OSGTrapezoidalShadowMapEngine.h>
#include <OpenSG/OSGImage.h>
#include <OpenSG/OSGLightEngine.h>
#include "core/objects/VRObjectFwd.h"
#include "core/scene/VRSceneFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

typedef LightEngine::LightTypeE LightTypeE;

class VRDefShading {
    private:
        enum ShadowTypeE {
            ST_NONE      = 0,
            ST_STANDARD  = 1,
            ST_TRAPEZOID = 2
        };

        struct LightInfo {
            VRLightWeakPtr vrlight;
            LightTypeE lightType;
            ShadowTypeE shadowType;
            LightMTRecPtr light;
            TextureObjChunkMTRecPtr texChunk;
            //NodeMTRecPtr               lightN;
            //NodeMTRecPtr               beaconN;

            ShaderProgramMTRecPtr      lightVP;
            ShaderProgramMTRecPtr      lightFP;
            ShaderProgramChunkMTRecPtr lightSH;
        };

        ShaderProgramMTRecPtr      vpAmbient;
        ShaderProgramMTRecPtr      fpAmbient;
        ShaderProgramChunkMTRecPtr shAmbient;

        VRLightPtr fogLight;

        string dsGBufferVPFile, dsGBufferFPFile;
        string dsAmbientVPFile, dsAmbientFPFile;
        string dsFogVPFile, dsFogFPFile;
        string dsDirLightVPFile, dsDirLightFPFile, dsDirLightShadowFPFile;
        string dsPointLightVPFile, dsPointLightFPFile, dsPointLightShadowFPFile;
        string dsPhotometricLightVPFile, dsPhotometricLightFPFile, dsPhotometricLightShadowFPFile;
        string dsSpotLightVPFile, dsSpotLightFPFile, dsSpotLightShadowFPFile;
        string dsUnknownFile = "unknownFile";

        TextureObjChunkRefPtr fboTex;
        NodeMTRecPtr dsStageN;
        DeferredShadingStageMTRecPtr dsStage;
        map<int, LightInfo> lightInfos;
        int shadowMapWidth;
        int shadowMapHeight;

        ShadowTypeE defaultShadowType;
        int shadowRes;
        Color4f shadowColor;
        Color4f fogParams;
        Color4f fogColor;
        bool initiated = false;
        bool enabled = false;
        int channel = 0;
        VRObjectPtr stageObject;
        VRMaterialPtr ssao_mat;

        void init();

    public:
        VRDefShading();
        ~VRDefShading();

        static VRDefShadingPtr create();

        void initDeferredShading(VRObjectPtr o);
        void setActive(bool b);
        void setDeferredShading(bool b);
        bool getDeferredShading();
        void reload();

        int addBuffer(int pformat, int ptype);
        void setDeferredChannel(int channel);
        TextureObjChunkRefPtr getTarget();
        DeferredShadingStageMTRecPtr getOSGStage();

        void setDSCamera(OSGCameraPtr cam);
        void addDSLight(VRLightPtr light);
        void updateLight(VRLightPtr l);
        void subLight(int ID);
        void setBackground(BackgroundRecPtr bg);

        const std::string &getLightVPFile(LightTypeE lightType);
        const std::string &getLightFPFile(LightTypeE lightType, ShadowTypeE shadowType);
};

OSG_END_NAMESPACE;

#endif // VRDEFSHADING_H_INCLUDED
