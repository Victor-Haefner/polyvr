#include "VRDeviceManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/utils/xml.h"
#include "core/objects/VRTransform.h"

#include "VRDevice.h"
#include "VRMouse.h"
#include "VRKeyboard.h"
#include "VRServer.h"
#include "addons/LeapMotion/VRLeap.h"

#ifndef WITHOUT_ART
#include "VRFlystick.h"
#endif

#ifndef WITHOUT_VIRTUOSE
#include "VRHaptic.h"
#endif

#ifndef WITHOUT_MTOUCH
#include "VRMultiTouch.h"
#endif

OSG_BEGIN_NAMESPACE;
using namespace std;

void VRDeviceManager::dev_test(VRDevicePtr dev) {
    int tmp = -1;
    float tmp2 = 0;
    int key = dev->key();
    dev->b_state(key, &tmp);
    dev->s_state(key, &tmp2);
    cout << "\nkey pressed : " << key << " state : " << tmp << " " << tmp2;
}

VRDeviceManager::VRDeviceManager() {
    cout << "Init VRDeviceManager\n";
}

VRDeviceManager::~VRDeviceManager() {
    cout << "~VRDeviceManager\n";
}

void VRDeviceManager::clearSignals() { for (auto dev : devices) dev.second->clearSignals(); }
void VRDeviceManager::setDeviceRoot(VRTransformPtr root) { device_root = root; }

void VRDeviceManager::updateDevices() {
    for (auto itr : devices) itr.second->updateBeacons();
}

void VRDeviceManager::addDevice(VRDevicePtr dev) {
    devices[dev->getName()] = dev;
    dev->getBeaconRoot()->switchParent(device_root);
    //dev->getCross()->switchParent(device_root); //TODO: add crosses as marker with a marker engine!
}

map<string, VRDevicePtr > VRDeviceManager::getDevices() { return devices; }

vector<string> VRDeviceManager::getDevices(string type) {
    vector<string> devs;
    for (auto itr : devices) if (itr.second->getType() == type) devs.push_back(itr.first);
    return devs;
}

vector<string> VRDeviceManager::getDeviceTypes() {
    map<string, int> types;
    vector<string> res;
    for (auto itr : devices) types[itr.second->getType()] = 0;
    for (auto t : types) res.push_back(t.first);
    return res;
}

/*VRDevicePtr VRDeviceManager::getDevice(string type, int i) { // deprecated?
    return 0;
}*/

VRDevicePtr VRDeviceManager::getDevice(string name) {
    if (devices.count(name) == 0) return 0;
    return devices[name];
}

void VRDeviceManager::updateActivatedSignals() {
    for (auto d : devices) d.second->updateSignals();
}

void VRDeviceManager::resetDeviceDynNodes(VRObjectPtr root) {
    for (auto d : devices) {
        d.second->clearDynTrees();
        d.second->addDynTree(root);
    }
}

void VRDeviceManager::save(XMLElementPtr node) {
    for (auto d : devices) {
        XMLElementPtr dn = node->addChild("Device");
        d.second->save(dn);
    }
}

void VRDeviceManager::load(XMLElementPtr node) {
    cout << "  load devices:" << endl;
    for (auto el : node->getChildren()) {
        if (!el) continue;

        string type = el->getAttribute("type");
        VRDevicePtr dev = 0;
        cout << "   new device, type: " << type << endl;

        if (type == "mouse") {
            VRMousePtr m = VRMouse::create();
            m->load(el);
            dev = m;
        }

#ifndef WITHOUT_MTOUCH
        if (type == "multitouch") {
            VRMultiTouchPtr m = VRMultiTouch::create();
            m->load(el);
            dev = m;
        }
#endif

        if (type == "leap") {
            VRLeapPtr m = VRLeap::create();
            m->load(el);
            dev = m;
        }

        if (type == "keyboard") {
            VRKeyboardPtr k = VRKeyboard::create();
            k->load(el);
            dev = k;
        }

#ifndef WITHOUT_VIRTUOSE
        if (type == "haptic") {
            VRHapticPtr h = VRHaptic::create();
            h->load(el);
            dev = h;
        }
#endif

        if (type == "server") {
#ifdef WASM  // TODO: hack, hangs here somewhere..
            continue;
#endif
            VRServerPtr m = VRServer::create(5500);
            m->load(el);
            dev = m;
        }


        if (dev == 0) continue;
        addDevice(dev);
    }

    cout << "  all devices loaded" << endl << endl;
}

OSG_END_NAMESPACE;
