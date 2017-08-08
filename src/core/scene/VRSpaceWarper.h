#ifndef VRSPACEWARPER_H_INCLUDED
#define VRSPACEWARPER_H_INCLUDED

#include <OpenSG/OSGMatrix.h>
#include <string>
#include <vector>
#include "core/scene/VRSceneFwd.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSpaceWarper : public std::enable_shared_from_this<VRSpaceWarper> {
    private:
        VRCameraPtr cam;

    public:
        VRSpaceWarper();
        ~VRSpaceWarper();

        static VRSpaceWarperPtr create();
        VRSpaceWarperPtr ptr();

        void setCamera(VRCameraPtr cam);
        void warp(Matrix4d& m);
};

OSG_END_NAMESPACE;

#endif // VRSPACEWARPER_H_INCLUDED
