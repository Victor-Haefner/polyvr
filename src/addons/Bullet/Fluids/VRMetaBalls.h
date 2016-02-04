#ifndef VRMETABALLS_H_INCLUDED
#define VRMETABALLS_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/objects/object/VRObject.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMetaBalls : public VRObject {
    private:
        VRStagePtr stage;
        VRMaterialPtr mat;

        void init();

    public:
        VRMetaBalls(string name);

        static VRMetaBallsPtr create(string name);

        void addChild(VRObjectPtr child, bool osg = true, int place = -1);

        VRMaterialPtr getMaterial();
};

OSG_END_NAMESPACE;

#endif // VRMETABALLS_H_INCLUDED
