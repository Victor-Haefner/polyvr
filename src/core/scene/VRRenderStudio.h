#ifndef VRRENDERSTUDIO_H_INCLUDED
#define VRRENDERSTUDIO_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGBackground.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGProjectionCameraDecorator.h>
#include "core/objects/VRObjectFwd.h"
#include "core/scene/VRSceneFwd.h"
#include "core/utils/VRStorage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDefShading;
class VRSSAO;
class VRHMDDistortion;

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

        VRMaterialPtr dsProxy_mat1;
        VRMaterialPtr dsProxy_mat2;
        VRMaterialPtr blur_mat;
        VRMaterialPtr ssao_mat;

        VRMaterialPtr calib_mat;
        VRMaterialPtr marker_mat;
        VRMaterialPtr hmdd_mat;

        map<string, VRGeometryPtr> renderLayer;
        VRMaterialPtr setupRenderLayer(string name, VRObjectPtr parent);

        void initDSProxy(VRMaterialPtr mat);
        void initCalib(VRMaterialPtr mat);
        void initMarker(VRMaterialPtr mat);

    protected:
        VRDefShading* defShading = 0;
        VRDefShading* defSSAO = 0;
        VRDefShading* defBlur = 0;
        VRSSAO* ssao = 0;
        VRHMDDistortion* hmdd = 0;
        VRObjectPtr root_system = 0;
        VRObjectPtr root_post_processing = 0;
        VRObjectPtr root_def_shading = 0;
        VRObjectPtr root_def_ssao = 0;
        VRObjectPtr root_def_blur = 0;
        map<int, VRLightPtr> light_map;

    public:
        VRRenderStudio(EYE e);
        ~VRRenderStudio();

        static VRRenderStudioPtr create(EYE e = LEFT);

        void init(VRObjectPtr root = 0);
        void reset();

        VRObjectPtr getRoot();

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
