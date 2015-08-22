#ifndef VRDEFSHADING_H_INCLUDED
#define VRDEFSHADING_H_INCLUDED

#include <OpenSG/OSGDeferredShadingStage.h>
#include <OpenSG/OSGShaderProgramChunk.h>
#include <OpenSG/OSGShaderProgram.h>
#include <OpenSG/OSGLight.h>

#include <OpenSG/OSGShaderShadowMapEngine.h>
#include <OpenSG/OSGTrapezoidalShadowMapEngine.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRLight;
class VRCamera;
class VRObject;

class VRDefShading {
    private:
        typedef LightEngine::LightTypeE LightTypeE;

        enum ShadowTypeE {
            ST_NONE      = 0,
            ST_STANDARD  = 1,
            ST_TRAPEZOID = 2
        };

        struct LightInfo {
            LightEngine::LightTypeE    lightType;
            ShadowTypeE                shadowType;
            LightRecPtr              light;
            //NodeRecPtr               lightN;
            //NodeRecPtr               beaconN;

            ShaderProgramRecPtr      lightVP;
            ShaderProgramRecPtr      lightFP;
            ShaderProgramChunkRecPtr lightSH;


        };

        NodeRecPtr                   dsStageN;
        DeferredShadingStageRecPtr   dsStage;

        vector<LightInfo>         lightInfos;


        Int32                          currentLight;

        UInt32                         shadowMapWidth;
        UInt32                         shadowMapHeight;

        ShadowTypeE defaultShadowType;
        int shadowRes;
        float shadowColor;

        void init();

    protected:
        bool dsInit;

        void initDeferredShading(VRObject* o);

    public:
        VRDefShading();

        void setDSCamera(VRCamera* cam);
        void addDSLight(VRLight* light);
        void addDSLight(LightRecPtr light, string type, bool shadows = false);
        void subLight(UInt32 lightIdx);
        void setShadow(LightInfo &li);

        const std::string &getLightVPFile(LightEngine::LightTypeE lightType);
        const std::string &getLightFPFile( LightEngine::LightTypeE lightType, ShadowTypeE shadowType);
};

OSG_END_NAMESPACE;

#endif // VRDEFSHADING_H_INCLUDED
