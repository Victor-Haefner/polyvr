#ifndef VRMETABALLS_H_INCLUDED
#define VRMETABALLS_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/objects/object/VRObject.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMetaBalls : public VRObject {
    private:
        VRStagePtr stage;
        VRStagePtr dstage;
        VRMaterialPtr mat;
        VRMaterialPtr depth_mat;

        void init();

    public:
        VRMetaBalls(string name);

        static VRMetaBallsPtr create(string name);

        void addChild(VRObjectPtr child, bool osg = true, int place = -1);

        VRMaterialPtr getMaterial();
        VRMaterialPtr getDepthMaterial();
};

OSG_END_NAMESPACE;

#endif // VRMETABALLS_H_INCLUDED
