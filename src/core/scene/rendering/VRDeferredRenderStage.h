#ifndef VRDEFERREDRENDERSTAGE_H_INCLUDED
#define VRDEFERREDRENDERSTAGE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include "core/objects/VRObjectFwd.h"
#include "core/scene/VRSceneFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDefShading;

class VRDeferredRenderStage {
    private:
        VRObjectPtr root;
        VRGeometryPtr layer;
        VRMaterialPtr mat;
        shared_ptr<VRDefShading> defRendering;
        shared_ptr<VRDeferredRenderStage> child;

        VRMaterialPtr setupRenderLayer(string name);

    public:
        VRDeferredRenderStage(string name);
        ~VRDeferredRenderStage();

        static VRDeferredRenderStagePtr create(string name);

        void setCamera(OSGCameraPtr cam);
        void addLight(VRLightPtr l);

        VRObjectPtr getTop();
        VRObjectPtr getBottom();
        VRMaterialPtr getMaterial();
        VRGeometryPtr getLayer();
        void initDeferred();
        shared_ptr<VRDefShading> getRendering();
        void setActive(bool da, bool la);
        void insert(shared_ptr<VRDeferredRenderStage> stage);
};

OSG_END_NAMESPACE;

#endif // VRDEFERREDRENDERSTAGE_H_INCLUDED
