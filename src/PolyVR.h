#ifndef VRFRAMEWORK_H_INCLUDED
#define VRFRAMEWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <list>

#include "PolyVRFwd.h"
#include "core/utils/VRFunctionFwd.h"
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

        int argc = 0;
        char** argv = 0;
        bool doLoop = false;
        bool initiated = false;
        list<VRUpdateCbPtr> initQueue;
        list<VRUpdateCbPtr>::iterator initQueueItr;

        void checkProcessesAndSockets();

        void initEnvironment();
        void initOpenSG();
        void initManagers();
        void initUI();
        void initFinalize();

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
