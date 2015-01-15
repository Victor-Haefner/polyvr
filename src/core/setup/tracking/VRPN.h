#ifndef VRPN_H_INCLUDED
#define VRPN_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGMatrix.h>
#include <string>
#include <map>

#include "core/utils/VRName.h"
#include "core/utils/VRStorage.h"

namespace xmlpp{ class Element; }
class vrpn_Tracker_Remote;
class vrpn_Connection;
template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRTransform;
class VRThread;

struct VRPN_tracker : public VRName, public VRStorage {
    VRTransform* ent = 0;
    string tracker;
    Vec3f offset;
    float scale = 1;
    vrpn_Tracker_Remote* vrpnt = 0;
    vrpn_Connection* vrpnc = 0;
    int ID = 0;

    VRPN_tracker();
    void setTracker(string t);
};

class VRPN : public VRStorage {
    private:
        map<int, VRPN_tracker*> tracker;//pointer map auf die objecte
        int threadID;
        bool active = true;

        //update thread
        void update(VRThread* thread);

    public:
        VRPN();
        ~VRPN();

        void addVRPNTracker(int ID, string addr, Vec3f offset, float scale);
        void delVRPNTracker(VRPN_tracker* t);

        VRFunction<int>* getVRPNUpdateFkt();

        vector<int> getVRPNTrackerIDs();
        VRPN_tracker* getVRPNTracker(int ID);
        void setVRPNActive(bool b);
        bool getVRPNActive();

        //void save(xmlpp::Element* node);
        //void load(xmlpp::Element* node);
};

OSG_END_NAMESPACE

#endif // VRPN_H_INCLUDED
