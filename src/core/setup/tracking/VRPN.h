#ifndef VRPN_H_INCLUDED
#define VRPN_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGMatrix.h>
#include <string>
#include <map>

#include "core/setup/devices/VRDevice.h"
#include "core/utils/VRFunctionFwd.h"

class vrpn_Tracker_Remote;
class vrpn_Button_Remote;
class vrpn_Analog_Remote;
class vrpn_Dial_Remote;
class vrpn_Text_Receiver;
class vrpn_Connection;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRPN_device : public VRDevice {
    public:
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
        ~VRPN_device();

        static VRPN_devicePtr create();
        VRPN_devicePtr ptr();

        void setAddress(string t);
        void setTranslationAxis(Vec3f v);
        void setRotationAxis(Vec3f v);
        void loop();
};

class VRPN : public VRStorage {
    private:
        map<int, VRPN_devicePtr> devices;//pointer map auf die objecte
        int threadID;
        bool active = true;
        int port = 3883;

        VRUpdatePtr updatePtr;
        VRUpdatePtr testServer = 0;

        //update thread
        void update_t(VRThread* thread);
        void update();

    public:
        VRPN();
        ~VRPN();

        void addVRPNTracker(int ID, string addr, Vec3f offset, float scale);
        void delVRPNTracker(VRPN_devicePtr t);

        VRFunction<int>* getVRPNUpdateFkt();

        vector<int> getVRPNTrackerIDs();
        VRPN_devicePtr getVRPNTracker(int ID);
        void setVRPNActive(bool b);
        bool getVRPNActive();

        void setVRPNPort(int p);
        int getVRPNPort();

        void changeVRPNDeviceName(VRPN_devicePtr dev, string name);

        void setVRPNVerbose(bool b);

        void startVRPNTestServer();
        void stopVRPNTestServer();
};

OSG_END_NAMESPACE

#endif // VRPN_H_INCLUDED
