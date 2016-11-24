#ifndef VRDEFERREDRENDERSTAGE_H_INCLUDED
#define VRDEFERREDRENDERSTAGE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDefShading;

class VRDeferredRenderStage {
    private:
        VRObjectPtr root;
        VRGeometryPtr layer;
        VRMaterialPtr mat;
        shared_ptr<VRDefShading> defRendering;

        VRMaterialPtr setupRenderLayer(string name);
        void initDeferred();

    public:
        VRDeferredRenderStage(string name);
        ~VRDeferredRenderStage();

        VRObjectPtr getTop();
        VRObjectPtr getBottom();
        VRMaterialPtr getMaterial();
        VRGeometryPtr getLayer();
        shared_ptr<VRDefShading> getRendering();
        void setActive(bool da, bool la);
};

OSG_END_NAMESPACE;

#endif // VRDEFERREDRENDERSTAGE_H_INCLUDED
