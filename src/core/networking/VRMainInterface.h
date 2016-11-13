#ifndef VRMAININTERFACE_H_INCLUDED
#define VRMAININTERFACE_H_INCLUDED

#include "OpenSG/OSGConfig.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include <string>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRServer;
class VRDevice;

class VRMainInterface {
    private:
        VRServerPtr server;
        string page;
        VRDeviceCb clickCb;

        VRMainInterface();
        void on_scene_clicked(VRDeviceWeakPtr dev);
        void update();

    public:
        static VRMainInterface* get();
        ~VRMainInterface();
};

OSG_END_NAMESPACE;

#endif // VRMAININTERFACE_H_INCLUDED
