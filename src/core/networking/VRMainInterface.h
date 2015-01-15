#ifndef VRMAININTERFACE_H_INCLUDED
#define VRMAININTERFACE_H_INCLUDED

#include "OpenSG/OSGConfig.h"
#include <string>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRMobile;
class VRDevice;

class VRMainInterface {
    private:
        VRMobile* mobile;
        string page;

        VRMainInterface();
        void on_scene_clicked(VRDevice* dev);
        void update();

    public:
        static VRMainInterface* get();
        ~VRMainInterface();
};

OSG_END_NAMESPACE;

#endif // VRMAININTERFACE_H_INCLUDED
