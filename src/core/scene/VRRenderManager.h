#ifndef VRRENDERMANAGER_H_INCLUDED
#define VRRENDERMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/utils/VRStorage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRLight;
class VRDefShading;
class VRObject;
class VRCamera;

class VRRenderManager : public VRStorage {
    private:
        bool frustumCulling = true;
        bool occlusionCulling = false;
        bool twoSided = true;
        bool deferredRendering = false;
        bool ssao = false;

    protected:
        VRDefShading* defShading = 0;
        VRObject* root = 0;
        VRObject* root_def_shading = 0;
        VRObject* root_ssao = 0;
        VRObject* root_system = 0;
        map<int, VRLight*> light_map;

    public:
        VRRenderManager();
        ~VRRenderManager();

        VRLight* addLight(string name);
        VRLight* getLight(int ID);

        void setFrustumCulling(bool b);
        bool getFrustumCulling();

        void setOcclusionCulling(bool b);
        bool getOcclusionCulling();

        void setTwoSided(bool b);
        bool getTwoSided();

        void setDSCamera(VRCamera* cam);
        void setDefferedShading(bool b);
        bool getDefferedShading();

        void setSSAO(bool b);
        bool getSSAO();

        void update();
};

OSG_END_NAMESPACE;

#endif // VRRENDERMANAGER_H_INCLUDED
