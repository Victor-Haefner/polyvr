#ifndef VRFRAMEWORK_H_INCLUDED
#define VRFRAMEWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>

#include "PolyVRFwd.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

template<class ValueTypeT, unsigned int SizeI> class Vector;
typedef Vector< double, 3 > Vec3d;

class PolyVR {
    private:
        VRInternalMonitorPtr monitor;
        VRGuiManagerPtr gui_mgr;
        VRMainInterfacePtr main_interface;
        VRSceneLoaderPtr loader;
        VRSetupManagerPtr setup_mgr;
        VRSceneManagerPtr scene_mgr;
        VRSoundManagerPtr sound_mgr;
        VROptionsPtr options;

        bool doLoop = false;

        void checkProcessesAndSockets();

    public:
        PolyVR();
        ~PolyVR();

        static shared_ptr<PolyVR> create();
        static PolyVR* get();
        static void shutdown();

        void run();
        void update();
        void init(int argc, char **argv);
        void startTestScene(OSGObjectPtr n, const Vec3d& camPos);
};

OSG_END_NAMESPACE;

#endif // VRFRAMEWORK_H_INCLUDED
