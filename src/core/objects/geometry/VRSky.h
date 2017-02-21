#ifndef VRSKY_H_INCLUDED
#define VRSKY_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/scene/VRSceneManager.h"
#include "VRGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSky : public VRGeometry {
    protected:
        VRUpdateCbPtr updatePtr;

        void update();
        Vec3f sunFromTime();

    public:
        VRSky();
        ~VRSky();

        static VRSkyPtr create();
        VRSkyPtr ptr();
        void setTime();
};

OSG_END_NAMESPACE;

#endif // VRSKY_H_INCLUDED
