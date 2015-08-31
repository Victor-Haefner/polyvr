#ifndef VRRENDERMANAGER_H_INCLUDED
#define VRRENDERMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/utils/VRStorage.h"
#include "VRDefShading.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRLight;

class VRRenderManager : public VRStorage, public VRDefShading {
    private:
        bool frustumCulling = true;
        bool occlusionCulling = false;
        bool twoSided = true;
        bool deferredRendering = false;
        bool ssao = false;

    protected:
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

        void update();
};

OSG_END_NAMESPACE;

#endif // VRRENDERMANAGER_H_INCLUDED
