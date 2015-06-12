#include "ART.h"
#include "core/scene/VRThreadManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/utils/VROptions.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/toString.h"
#include <libxml++/nodes/element.h>
#include "DTrack.h"
#include "core/setup/devices/VRFlystick.h"
#include "core/utils/VRFunction.h"
#include "core/objects/VRTransform.h"
#include "core/math/coordinates.h"
#include "core/utils/VRStorage_template.h"
#include "core/setup/devices/VRSignal.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

ART_device::ART_device() { setName("ARTDevice"); }
ART_device::ART_device(int ID, int type) : ID(ID), type(type) { setName("ARTDevice"); init(); }
int ART_device::key() { return key(ID, type); }
int ART_device::key(int ID, int type) { return ID*1000 + type; }

void ART_device::init() {
    if (type != 1) ent = new VRTransform("ART_tracker");
    if (type == 1) {
        dev = new VRFlystick();
        ent = dev->editBeacon();
        VRSetupManager::getCurrent()->addDevice(dev);
    }
}

void ART_device::update() {
    if (ent) ent->setMatrix(m);
    if (dev) {
        auto bitr = buttons.begin();
        auto jitr = joysticks.begin();
        for (; bitr != buttons.end() && jitr != joysticks.end(); bitr++, jitr++) {
            dev->update(*bitr, *jitr);
        }
        buttons.clear();
        joysticks.clear();
    }
}


ART::ART() {
    auto fkt  = new VRFunction<int>("ART_apply", boost::bind(&ART::applyEvents, this));
    VRSceneManager::get()->addUpdateFkt(fkt);

    auto fkt2 = new VRFunction<VRThread*>("ART_fetch", boost::bind(&ART::updateT, this, _1));
    VRSceneManager::get()->initThread(fkt2, "ART_fetch", true);

    on_new_device = new VRSignal();

    store("active", &active);
    store("port", &port);
    store("offset", &offset);
    store("up", &up);
}

ART::~ART() {}

template<typename dev>
void ART::getMatrix(dev t, ART_device* d) {
    if (t.quality <= 0) return;

    Matrix& m = d->m;
    m[0] = Vec4f(t.rot[0], t.rot[1], t.rot[2], 1); // orientation
    m[1] = Vec4f(t.rot[3], t.rot[4], t.rot[5], 1);
    m[2] = Vec4f(t.rot[6], t.rot[7], t.rot[8], 1);

    m[1] = Vec4f(t.rot[6], t.rot[7], t.rot[8], 1); // test
    m[2] = Vec4f(-t.rot[3], -t.rot[4], -t.rot[5], 1);

    m[3] = Vec4f(t.loc[0]*0.001, t.loc[1]*0.001, t.loc[2]*0.001, 1); // position
    coords::YtoZ(m); // LESC -> TODO: use the up value and others to specify the coordinate system
    m[3] += Vec4f(d->offset) + Vec4f(offset);
}

void ART::scan(int type, int N) {
    if (type < 0) {
        boost::mutex::scoped_lock lock(mutex);
        scan(0, dtrack->get_num_body());
        scan(1, dtrack->get_num_flystick());
        scan(2, dtrack->get_num_hand());
        scan(3, dtrack->get_num_meatool());
        //scan(4, dtrack->get_num_marker());
        return;
    }

    for (int i=0; i<N; i++) {
        int k = ART_device::key(i,type);
        if (devices.count(k) == 0) continue;

        if (type == 0) getMatrix(dtrack->get_body(i), devices[k]);
        if (type == 1) getMatrix(dtrack->get_flystick(i), devices[k]);
        if (type == 2) getMatrix(dtrack->get_hand(i), devices[k]);
        if (type == 3) getMatrix(dtrack->get_meatool(i), devices[k]);

        if (type == 1) {
            auto fly = dtrack->get_flystick(i);
            devices[k]->buttons.push_back( vector<int>(fly.button, &fly.button[fly.num_button]) );
            devices[k]->joysticks.push_back( vector<float>(fly.joystick, &fly.joystick[fly.num_joystick]) );
        }
    }
}

void ART::updateL() { updateT(0); }

//update thread
void ART::updateT(VRThread* t) {
    if (!active) {
        osgSleep(1);
        return;
    }

    setARTPort(port);
    if (!active || dtrack == 0) return;

    if (dtrack->receive()) scan();
    else {
        if(dtrack->timeout())       cout << "--- ART: timeout while waiting for udp data" << endl;
        if(dtrack->udperror())      cout << "--- ART: error while receiving udp data" << endl;
        if(dtrack->parseerror())    cout << "--- ART: error while parsing udp data" << endl;
    }
}

void ART::checkNewDevices(int type, int N) {
    setARTPort(port);
    if (!active || dtrack == 0) return;

    //check for new devices
    if (type < 0) {
        checkNewDevices(0, dtrack->get_num_body());
        checkNewDevices(1, dtrack->get_num_flystick());
        checkNewDevices(2, dtrack->get_num_hand());
        checkNewDevices(3, dtrack->get_num_meatool());
        //checkNewDevices(4, dtrack->get_num_marker());
        return;
    }

    for (int i=0; i<N; i++) {
        int k = ART_device::key(i,type);
        if (devices.count(k) == 0) {
            devices[k] = new ART_device(i,type);
            VRSetupManager::getCurrent()->getSignal_on_new_art_device()->trigger<VRDevice>();
        }
    }
}

void ART::applyEvents() {
    boost::mutex::scoped_lock lock(mutex);
    checkNewDevices();
    for (auto d : devices) d.second->update();
}

vector<int> ART::getARTDevices() {
    vector<int> devs;
    for (auto& itr : devices) devs.push_back(itr.first);
    return devs;
}

ART_device* ART::getARTDevice(int dev) { return devices[dev]; }

void ART::setARTActive(bool b) { active = b; }
bool ART::getARTActive() { return active; }

int ART::getARTPort() { return port; }
void ART::setARTPort(int port) {
    if (current_port == port) return;
    this->port = port;
    current_port = port;

    if (dtrack != 0) delete dtrack;
    dtrack = new DTrack(port);
    if (!dtrack->valid()) {
        cout << "DTrack init error" << endl;
        delete dtrack;
        port = -1;
        dtrack = 0;
    }
    dtrack->receive();
}

void ART::setARTOffset(Vec3f o) { offset = o; }
Vec3f ART::getARTOffset() { return offset; }

void ART::startTestStream() {
    // TODO: create test data
}

VRSignal* ART::getSignal_on_new_art_device() { return on_new_device; }

OSG_END_NAMESPACE
