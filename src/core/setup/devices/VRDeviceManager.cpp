#include "VRDeviceManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/objects/VRTransform.h"
#include <libxml++/nodes/element.h>

#include "VRDevice.h"
#include "VRMouse.h"
#include "VRMultiTouch.h"
#include "VRKeyboard.h"
#include "VRFlystick.h"
#include "VRHaptic.h"
#include "VRServer.h"

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
    for (auto itr : devices) itr.second->updateBeacon();
}

void VRDeviceManager::addDevice(VRDevicePtr dev) {
    devices[dev->getName()] = dev;
    dev->getBeacon()->switchParent(device_root);
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

void VRDeviceManager::save(xmlpp::Element* node) {
    xmlpp::Element* dn;
    for (auto d : devices) {
        dn = node->add_child("Device");
        d.second->save(dn);
    }
}

void VRDeviceManager::load(xmlpp::Element* node) {
    cout << "Load devices:";
    xmlpp::Node::NodeList nl = node->get_children();
    xmlpp::Node::NodeList::iterator itr;
    for (itr = nl.begin(); itr != nl.end(); itr++) {
        xmlpp::Node* n = *itr;

        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        string type = el->get_attribute("type")->get_value();
        VRDevicePtr dev = 0;
        cout << " " << type;

        if (type == "mouse") {
            VRMousePtr m = VRMouse::create();
            m->load(el);
            dev = m;
        }

        if (type == "multitouch") {
            VRMultiTouchPtr m = VRMultiTouch::create();
            m->load(el);
            dev = m;
        }

        if (type == "keyboard") {
            VRKeyboardPtr k = VRKeyboard::create();
            k->load(el);
            dev = k;
        }

        if (type == "haptic") {
            VRHapticPtr h = VRHaptic::create();
            h->load(el);
            dev = h;
        }

        if (type == "server") {
            VRServerPtr m = VRServer::create(5500);
            m->load(el);
            dev = m;
        }


        if (dev == 0) continue;
        //cout << "\nMouse dev " << dev << flush;

        addDevice(dev);
    }

    cout << endl;
}

OSG_END_NAMESPACE;
