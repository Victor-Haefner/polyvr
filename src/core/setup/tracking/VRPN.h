#ifndef VRPN_H_INCLUDED
#define VRPN_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGMatrix.h>
#include <string>
#include <map>

namespace xmlpp{ class Element; }
class vrpn_Tracker_Remote;
class vrpn_Connection;
template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRTransform;
class VRThread;

struct VRPN_tracker {
    VRTransform* ent;
    string tracker;
    Vec3f offset;
    float scale;
    vrpn_Tracker_Remote* vrpnt;
    vrpn_Connection* vrpnc;
    int ID;

    VRPN_tracker();
    void setTracker(string t);
};

class VRPN {
    private:
        map<int, VRPN_tracker*> tracker;//pointer map auf die objecte
        int threadID;

        //update thread
        void update(VRThread* thread);

    public:
        VRPN();
        ~VRPN();

        void addVRPNTracker(int ID, string addr, Vec3f offset, float scale);

        VRFunction<int>* getVRPNUpdateFkt();

        vector<int> getVRPNTrackerIDs();
        VRPN_tracker* getVRPNTracker(int ID);

        void save(xmlpp::Element* node);
        void load(xmlpp::Element* node);
};

OSG_END_NAMESPACE

#endif // VRPN_H_INCLUDED
