#ifndef VRDEVICEMANAGER_H_INCLUDED
#define VRDEVICEMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <vector>
#include <string>
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "core/utils/VRUtilsFwd.h"

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDeviceManager {
    private:
        map<string, VRDevicePtr > devices;// key is dev type

        void dev_test(VRDevicePtr dev);

        VRTransformPtr device_root;

    public:
        VRDeviceManager();
        ~VRDeviceManager();

        void clearSignals();

        void setDeviceRoot(VRTransformPtr root);
        void addDevice(VRDevicePtr dev);

        //VRDevicePtr getDevice(string type, int i);
        VRDevicePtr getDevice(string name);
        vector<string> getDevices(string type);
        vector<string> getDeviceTypes();
        map<string, VRDevicePtr > getDevices();

        void updateActivatedSignals();
        void resetDeviceDynNodes(VRObjectPtr ancestor);
        void updateDevices();

        void save(XMLElementPtr node);
        void load(XMLElementPtr node);
};

OSG_END_NAMESPACE;

#endif // VRDEVICEMANAGER_H_INCLUDED
