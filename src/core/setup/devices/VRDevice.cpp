#include "VRDevice.h"
#include "VRSignal.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRSceneManager.h"
#include <libxml++/nodes/element.h>
#include <boost/bind.hpp>

OSG_BEGIN_NAMESPACE;

VRSignal* VRDevice::signalExist(int key, int state) {
    stringstream ss;
    ss << "on_" << name << "_" << key << "_" << state;
    string sig_name = ss.str();
    if ( callbacks.count(sig_name) ==  0) return 0;
    return callbacks[sig_name];
}

VRSignal* VRDevice::createSignal(int key, int state) {
    stringstream ss;
    ss << "on_" << name << "_" << key << "_" << state;
    string sig_name = ss.str();

    if ( callbacks.count(sig_name) ==  0) callbacks[sig_name] = new VRSignal(this);
    VRSignal* sig = callbacks[sig_name];
    sig->setName(sig_name);
    return sig;
}

void VRDevice::triggerSignal(int key, int state) {
    VRSignal* sig = signalExist(key, state);
    if (sig) {
        sig->trigger<VRDevice>();
        if (sig->doUpdate()) addUpdateSignal(sig);
    }
}

VRSignal* VRDevice::getToEdgeSignal() { return 0; }
VRSignal* VRDevice::getFromEdgeSignal() { return 0; }

void VRDevice::addUpdateSignal(VRSignal* sig) {
    activatedSignals.push_back(sig);
}

void VRDevice::remUpdateSignal(VRSignal* sig, VRDevice* dev) {
    for (auto itr : activatedSignals) {
        if (itr == sig) {
            sig->trigger<VRDevice>();
            activatedSignals.erase(remove(activatedSignals.begin(), activatedSignals.end(), sig), activatedSignals.end());
            return;
        }
    }
}

void VRDevice::updateSignals() {
    for(uint i=0; i<activatedSignals.size(); i++) {
        activatedSignals[i]->trigger<VRDevice>();
    }
}

VRDevice::VRDevice(string _type) : VRAvatar(_type) {
    type = _type;
    setName(_type);

    store("type", &type);
    store("name", &name);
}

VRDevice::~VRDevice() {
    //activatedSignals;
}

void VRDevice::clearSignals() {
    for (auto itr : callbacks) delete itr.second;
    callbacks.clear();
    activatedSignals.clear();
}

string VRDevice::getType() { return type; }

// signal event setup
VRSignal* VRDevice::addSignal(int key, int state) {
    VRSignal* sig = createSignal(key, state);
    BStates[key] = false;
    return sig;
}

// signal activation setup
VRSignal* VRDevice::addSignal(int key) {
    BStates[key] = false;

    // activation signal
    VRSignal* sigA = createSignal(key, 1);
    sigA->setUpdate(true);

    // deactivation signal
    VRSignal* sigB = createSignal(key, 0);
    sigB->add( new VRDevCb("Device_remUpdate", boost::bind(&VRDevice::remUpdateSignal, this, sigA, _1)) );

    return sigA;
}

void VRDevice::addSignal(VRSignal* sig, int key, int state) {
    stringstream ss;
    ss << "on_" << name << "_" << key << "_" << state;
    sig->setName(ss.str());
    callbacks[sig->getName()] = sig;
    BStates[key] = false;
}

VRSignal* VRDevice::addSlider(int key) {
    VRSignal* sig = createSignal(key, 0);
    SStates[key] = 0;
    return sig;
}

void VRDevice::change_button(int key, int state) {
    if (BStates.count(key) == 0) BStates[key] = state;
    sig_key = key;
    sig_state = state;
    BStates[key] = state;
    triggerSignal(key, state);
    triggerSignal(-1, state);
}

void VRDevice::change_slider(int key, float state) {
    if (SStates.count(key) == 0) SStates[key] = state;
    SStates[key] = state;
    sig_key = key;
    sig_state = state;
    if (abs(state) > 0.001) triggerSignal(key, 0);
}

int VRDevice::key() { return sig_key; }
int VRDevice::getState() { return sig_state; }
string VRDevice::getMessage() { return message; }
void VRDevice::setMessage(string s) { message = s; }

void VRDevice::b_state(int key, int* b_state) { if (BStates.count(key)) *b_state = BStates[key]; }
int VRDevice::b_state(int key) { if (BStates.count(key)) return BStates[key]; else return -1; }

void VRDevice::s_state(int key, float* s_state) { if (SStates.count(key)) *s_state = SStates[key];}
float VRDevice::s_state(int key) { if (SStates.count(key)) return SStates[key]; else return 0; }

void VRDevice::setTarget(VRTransform* e) { target = e; }
VRTransform* VRDevice::getTarget() { return target; }

map<string, VRSignal*> VRDevice::getSignals() { return callbacks; }
VRSignal* VRDevice::getSignal(string name) { if (callbacks.count(name)) return callbacks[name]; else return 0; }

void VRDevice::printMap() {
    cout << "\nDevice " << name << " signals:";
    for (auto c : callbacks) cout << "\n Signal " << c.second->getName();
    for (auto b : BStates) cout << "\n BState key " << b.first << " " << b.second;
    for (auto s : SStates) cout << "\n SState key " << s.first << " " << s.second;
    cout << endl;
}

void VRDevice::setSpeed(Vec2f s) { speed = s; }
Vec2f VRDevice::getSpeed() { return speed; }

OSG_END_NAMESPACE;
