#ifndef VRFRAMEWORK_H_INCLUDED
#define VRFRAMEWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <string>

#include "PolyVRFwd.h"
#include "core/objects/VRObjectFwd.h"

class VROptions;

OSG_BEGIN_NAMESPACE;
using namespace std;

class PolyVR {
    private:
        VRInternalMonitorPtr monitor;
        VRGuiManagerPtr gui_mgr;
        VRMainInterfacePtr interface;
        VRSceneLoaderPtr loader;
        VRSetupManagerPtr setup_mgr;
        VRSceneManagerPtr scene_mgr;
        VRSoundManagerPtr sound_mgr;
        VROptionsPtr options;

        void setMultisampling(bool on);
        void checkProcessesAndSockets();

    public:
        PolyVR();
        ~PolyVR();
        static PolyVR* get();
        static void shutdown();

        void run();
        void update();
        void init(int argc, char **argv);
        void startTestScene(OSGObjectPtr n, Vec3d camPos);
};

OSG_END_NAMESPACE;

#endif // VRFRAMEWORK_H_INCLUDED
