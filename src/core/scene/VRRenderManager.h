#ifndef VRRENDERMANAGER_H_INCLUDED
#define VRRENDERMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/utils/VRStorage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRRenderManager : public VRStorage {
    private:
        bool frustumCulling;
        bool occlusionCulling;

    public:
        VRRenderManager();
        ~VRRenderManager();

        void setFrustumCulling(bool b);
        bool getFrustumCulling();

        void setOcclusionCulling(bool b);
        bool getOcclusionCulling();

        void update();
};

OSG_END_NAMESPACE;

#endif // VRRENDERMANAGER_H_INCLUDED
