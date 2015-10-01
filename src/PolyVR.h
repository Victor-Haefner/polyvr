#ifndef VRFRAMEWORK_H_INCLUDED
#define VRFRAMEWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <memory>

class VROptions;

OSG_BEGIN_NAMESPACE;
using namespace std;

class Node;
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

        shared_ptr<VROptions> options;
        shared_ptr<VRSceneManager> scene_mgr;
        shared_ptr<VRSetupManager> setup_mgr;
        shared_ptr<VRInternalMonitor> monitor;
        shared_ptr<VRGuiManager> gui_mgr;
        shared_ptr<VRMainInterface> interface;
        shared_ptr<VRSceneLoader> loader;
        shared_ptr<VRSoundManager> sound_mgr;

        PolyVR();

    public:
        ~PolyVR();
        static PolyVR& get();

        void init(int argc, char **argv);

        void start();
        void exit();

        void startTestScene(Node* n);
};

OSG_END_NAMESPACE;

#endif // VRFRAMEWORK_H_INCLUDED
