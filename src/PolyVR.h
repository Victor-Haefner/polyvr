#ifndef VRFRAMEWORK_H_INCLUDED
#define VRFRAMEWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"

class VROptions;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSceneManager;
class VRSetupManager;
class VRInternalMonitor;
class VRGuiManager;
class VRMainInterface;
class VRSceneLoader;
class VRSoundManager;

class PolyVR {
    private:
        void setMultisampling(bool on);

        shared_ptr<VRInternalMonitor> monitor;
        shared_ptr<VRGuiManager> gui_mgr;
        shared_ptr<VRMainInterface> interface;
        shared_ptr<VRSceneLoader> loader;
        shared_ptr<VRSetupManager> setup_mgr;
        shared_ptr<VRSceneManager> scene_mgr;
        shared_ptr<VRSoundManager> sound_mgr;
        shared_ptr<VROptions> options;

        void checkProcessesAndSockets();

        void run();

    public:
        PolyVR();
        ~PolyVR();
        static PolyVR* get();
        static void shutdown();

        void init(int argc, char **argv);
        void start(bool runit = true);
        void startTestScene(OSGObjectPtr n);

        void setOption(string name, bool val);
        void setOption(string name, string val);
        void setOption(string name, int val);
        void setOption(string name, float val);
};

OSG_END_NAMESPACE;

#endif // VRFRAMEWORK_H_INCLUDED
