#ifndef VRRENDERMANAGER_H_INCLUDED
#define VRRENDERMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRStorage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRLight;
class VRDefShading;
class VRSSAO;
class VRObject;
class VRCamera;

class VRRenderManager : public VRStorage {
    private:
        bool frustumCulling = true;
        bool occlusionCulling = false;
        bool twoSided = true;
        bool deferredRendering = false;
        bool do_ssao = false;
        bool calib = false;
        int ssao_kernel = 4;
        int ssao_noise = 4;
        float ssao_radius = 0.02;

        map<string, VRGeometryPtr> renderLayer;
        VRMaterialPtr setupRenderLayer(string name);

        void initCalib(VRMaterialPtr mat);

    protected:
        VRDefShading* defShading = 0;
        VRSSAO* ssao = 0;
        VRObjectPtr root = 0;
        VRObjectPtr root_def_shading = 0;
        VRObjectPtr root_system = 0;
        map<int, VRLightPtr> light_map;

    public:
        VRRenderManager();
        ~VRRenderManager();

        void addLight(VRLightPtr l);
        VRLightPtr getLight(int ID);

        void setFrustumCulling(bool b);
        bool getFrustumCulling();

        void setOcclusionCulling(bool b);
        bool getOcclusionCulling();

        void setTwoSided(bool b);
        bool getTwoSided();

        void setDSCamera(VRCameraPtr cam);
        void setDefferedShading(bool b);
        bool getDefferedShading();

        void setSSAO(bool b);
        bool getSSAO();
        void setSSAOradius(float r);
        void setSSAOkernel(int k);
        void setSSAOnoise(int n);

        void setCalib(bool b);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRRENDERMANAGER_H_INCLUDED
