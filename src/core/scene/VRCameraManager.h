#ifndef VRCAMERAMANAGER_H_INCLUDED
#define VRCAMERAMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <vector>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


class VRCameraManager {
    private:
        VRCameraWeakPtr active;

    public:
        VRCameraManager();
        ~VRCameraManager();

        VRTransformPtr addCamera(string name);
        void addCamera(VRCameraPtr cam);

        VRCameraPtr getCamera(int ID);

        void setMActiveCamera(string cam);

        VRCameraPtr getActiveCamera();
        int getActiveCameraIndex();

        vector<string> getCameraNames();
};

OSG_END_NAMESPACE;

#endif // VRCAMERAMANAGER_H_INCLUDED
