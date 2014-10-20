#ifndef VRDATAMANAGER_H_INCLUDED
#define VRDATAMANAGER_H_INCLUDED


//#include "Graphene.h"
//#include "Neutronenfluss.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDataManager {
    private:
        VRDataManager() { ; }

        void operator= (VRDataManager v) {;}

    public:
        static VRDataManager* get() {
            static VRDataManager* singleton = 0;
            if (singleton == 0) singleton = new VRDataManager();
            return singleton;
        }

        void startGraphene() {
//            new graphene(VRSceneManager::get()->getDeviceFlystick(), VRSceneManager::get()->getTrackerFlystick());
        }

        void startNeutronenfluss() {
//            new neutronenfluss(VRSceneManager::get()->getDeviceFlystick(), VRSceneManager::get()->getTrackerFlystick());
        }
};

OSG_END_NAMESPACE;


#endif // VRDATAMANAGER_H_INCLUDED
