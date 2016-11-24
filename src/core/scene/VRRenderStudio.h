#ifndef VRRENDERSTUDIO_H_INCLUDED
#define VRRENDERSTUDIO_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGBackground.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGProjectionCameraDecorator.h>
#include "core/objects/VRObjectFwd.h"
#include "core/scene/VRSceneFwd.h"
#include "core/utils/VRStorage.h"
#include "VRDeferredRenderStage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDefShading;
class VRSSAO;
class VRHMDDistortion;
class VRDeferredRenderStage;

class VRRenderStudio : public VRStorage {
    public:
        enum EYE {
            LEFT = 0,
            RIGHT
        };

    private:
        bool deferredRendering = false;
        bool do_ssao = false;
        bool calib = false;
        bool do_hmdd = false;
        bool do_marker = false;
        int ssao_kernel = 4;
        int ssao_noise = 4;
        float ssao_radius = 0.02;
        EYE eye = LEFT;

        map<string, std::shared_ptr<VRDeferredRenderStage>> stages;

        void initDSProxy(VRMaterialPtr mat);
        void initCalib(VRMaterialPtr mat);
        void initMarker(VRMaterialPtr mat);

    protected:
        shared_ptr<VRSSAO> ssao;
        shared_ptr<VRHMDDistortion> hmdd;
        VRObjectPtr root_system;
        VRObjectPtr root_scene;
        map<int, VRLightPtr> light_map;

    public:
        VRRenderStudio(EYE e);
        ~VRRenderStudio();

        static VRRenderStudioPtr create(EYE e = LEFT);

        void init(VRObjectPtr root = 0);
        void reset();

        VRObjectPtr getRoot();

        void addStage(string name, string parent = "");
        void setStageActive(string name, bool da, bool la);
        void setStageShader(string name, string VPpath, string FPpath, bool doDeferred);

        void addLight(VRLightPtr l);
        void updateLight(VRLightPtr l);
        VRLightPtr getLight(int ID);
        void subLight(VRLightPtr l);
        void clearLights();

        void setScene(VRObjectPtr root);
        void setCamera(VRCameraPtr cam);
        void setCamera(ProjectionCameraDecoratorRecPtr cam);
        void setBackground(BackgroundRecPtr bg);
        void resize(Vec2i s);
        void setEye(EYE e);

        void setDefferedShading(bool b);
        void setDeferredChannel(int c);
        bool getDefferedShading();
        void setSSAO(bool b);
        bool getSSAO();
        void setSSAOradius(float r);
        void setSSAOkernel(int k);
        void setSSAOnoise(int n);
        void setCalib(bool b);
        void setHMDD(bool b);
        bool getHMDD();
        void setMarker(bool b);
        bool getMarker();

        void setHMDDeye(float e);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRRENDERSTUDIO_H_INCLUDED
