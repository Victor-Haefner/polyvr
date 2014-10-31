#include "VRDeviceManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/objects/VRTransform.h"
#include <libxml++/nodes/element.h>

#include "VRDevice.h"
#include "VRMouse.h"
#include "VRKeyboard.h"
#include "VRFlystick.h"
#include "VRHaptic.h"
#include "VRMobile.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void VRDeviceManager::dev_test(VRDevice* dev) {
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

VRDeviceManager::~VRDeviceManager() { for (auto dev : devices) delete dev.second; }
void VRDeviceManager::clearSignals() { for (auto dev : devices) dev.second->clearSignals(); }
void VRDeviceManager::setDeviceRoot(VRTransform* root) { device_root = root; }

void VRDeviceManager::addDevice(VRDevice* dev) {
    devices[dev->getName()] = dev;
    dev->getBeacon()->switchParent(device_root);
    //dev->getCross()->switchParent(device_root); //TODO: add crosses as marker with a marker engine!
}

map<string, VRDevice* > VRDeviceManager::getDevices() { return devices; }

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

/*VRDevice* VRDeviceManager::getDevice(string type, int i) { // deprecated?
    return 0;
}*/

VRDevice* VRDeviceManager::getDevice(string name) {
    if (devices.count(name) == 0) return 0;
    return devices[name];
}

void VRDeviceManager::updateActivatedSignals() {
    for (itr =devices.begin(); itr!=devices.end(); itr++) {
        itr->second->updateSignals();
    }
}

void VRDeviceManager::updateDeviceDynNodes(VRObject* ancestor) {
    for (itr =devices.begin(); itr!=devices.end(); itr++) {
        itr->second->updateDynTree(ancestor);
    }
}

void VRDeviceManager::save(xmlpp::Element* node) {
    xmlpp::Element* dn;
    for (itr = devices.begin(); itr != devices.end(); itr++) {
        dn = node->add_child("Device");
        itr->second->save(dn);
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
        VRDevice* dev = 0;
        cout << " " << type;

        if (type == "mouse") {
            VRMouse* m = new VRMouse();
            m->load(el);
            dev = m;
        }

        if (type == "keyboard") {
            VRKeyboard* k = new VRKeyboard();
            k->load(el);
            dev = k;
        }

        if (type == "flystick") {
            VRFlystick* f = new VRFlystick();
            f->load(el);
            VRSetup* setup = (VRSetup*)this;
            setup->addARTDevice(f);
            dev = f;
        }

        if (type == "haptic") {
            VRHaptic* h = new VRHaptic();
            h->load(el);
            dev = h;
        }

        if (type == "mobile") {
            VRMobile* m = new VRMobile();
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
