#ifndef VRPN_H_INCLUDED
#define VRPN_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGMatrix.h>
#include <string>
#include <map>

#include "core/setup/devices/VRDevice.h"

namespace xmlpp{ class Element; }
class vrpn_Tracker_Remote;
class vrpn_Button_Remote;
class vrpn_Analog_Remote;
class vrpn_Dial_Remote;
class vrpn_Text_Receiver;
class vrpn_Connection;
template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRThread;

struct VRPN_device : public VRDevice {
    string address;
    Vec3f offset;
    float scale = 1;
    Vec3f translate_axis;
    Vec3f rotation_axis;
    vrpn_Tracker_Remote* tracker = 0;
	vrpn_Button_Remote*  button = 0;
	vrpn_Analog_Remote*  analog = 0;
	vrpn_Dial_Remote*    dial = 0;
    vrpn_Text_Receiver*  text = 0;

    vrpn_Connection* vrpnc = 0;
    int ID = 0;
    bool initialized = false;

    VRPN_device();
    void setAddress(string t);
    void setTranslationAxis(Vec3f v);
    void setRotationAxis(Vec3f v);
    void loop();
};

class VRPN : public VRStorage {
    private:
        map<int, VRPN_device*> devices;//pointer map auf die objecte
        int threadID;
        bool active = true;
        bool verbose = false;
        int port = 3883;

        VRFunction<int>* testServer = 0;

        //update thread
        void update_t(VRThread* thread);
        void update();

    public:
        VRPN();
        ~VRPN();

        void addVRPNTracker(int ID, string addr, Vec3f offset, float scale);
        void delVRPNTracker(VRPN_device* t);

        VRFunction<int>* getVRPNUpdateFkt();

        vector<int> getVRPNTrackerIDs();
        VRPN_device* getVRPNTracker(int ID);
        void setVRPNActive(bool b);
        bool getVRPNActive();

        void setVRPNPort(int p);
        int getVRPNPort();

        void changeVRPNDeviceName(VRPN_device* dev, string name);

        void setVRPNVerbose(bool b);

        void startVRPNTestServer();
        void stopVRPNTestServer();
};

OSG_END_NAMESPACE

#endif // VRPN_H_INCLUDED
