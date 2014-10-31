#ifndef VRDEVICEMANAGER_H_INCLUDED
#define VRDEVICEMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <vector>
#include <string>

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDevice;
class VRTransform;
class VRObject;

class VRDeviceManager {
    private:
        map<string, VRDevice* > devices;
        map<string, VRDevice* >::iterator itr; // key is dev type

        void dev_test(VRDevice* dev);

        VRTransform* device_root;

    public:
        VRDeviceManager();
        ~VRDeviceManager();

        void clearSignals();

        void setDeviceRoot(VRTransform* root);
        void addDevice(VRDevice* dev);

        //VRDevice* getDevice(string type, int i);
        VRDevice* getDevice(string name);
        vector<string> getDevices(string type);
        vector<string> getDeviceTypes();
        map<string, VRDevice* > getDevices();

        void updateActivatedSignals();
        void updateDeviceDynNodes(VRObject* ancestor);

        void save(xmlpp::Element* node);
        void load(xmlpp::Element* node);
};

OSG_END_NAMESPACE;

#endif // VRDEVICEMANAGER_H_INCLUDED
