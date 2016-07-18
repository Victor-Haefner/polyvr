#ifndef VRRENDERSTUDIO_H_INCLUDED
#define VRRENDERSTUDIO_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGBackground.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGProjectionCameraDecorator.h>
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRStorage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDefShading;
class VRSSAO;
class VRHMDDistortion;

class VRRenderStudio : public VRStorage {
    private:
        bool deferredRendering = false;
        bool do_ssao = false;
        bool calib = false;
        bool do_hmdd = false;
        int ssao_kernel = 4;
        int ssao_noise = 4;
        float ssao_radius = 0.02;

        map<string, VRGeometryPtr> renderLayer;
        VRMaterialPtr setupRenderLayer(string name, VRObjectPtr parent);

        void initCalib(VRMaterialPtr mat);

    protected:
        VRDefShading* defShading = 0;
        VRSSAO* ssao = 0;
        VRHMDDistortion* hmdd = 0;
        VRObjectPtr root_post_processing = 0;
        VRObjectPtr root_def_shading = 0;
        VRObjectPtr root_system = 0;
        VRObjectPtr root = 0;
        map<int, VRLightPtr> light_map;

    public:
        VRRenderStudio();
        ~VRRenderStudio();

        void init(VRObjectPtr root = 0);

        VRObjectPtr getRoot();

        void addLight(VRLightPtr l);
        VRLightPtr getLight(int ID);

        void setScene(VRObjectPtr root);
        void setCamera(VRCameraPtr cam);
        void setCamera(ProjectionCameraDecoratorRecPtr cam);
        void setBackground(BackgroundRecPtr bg);
        void resize(Vec2i s);

        void setDefferedShading(bool b);
        bool getDefferedShading();
        void setSSAO(bool b);
        bool getSSAO();
        void setSSAOradius(float r);
        void setSSAOkernel(int k);
        void setSSAOnoise(int n);
        void setCalib(bool b);
        void setHMDD(bool b);
        bool getHMDD();

        void update();
};

OSG_END_NAMESPACE;

#endif // VRRENDERSTUDIO_H_INCLUDED
