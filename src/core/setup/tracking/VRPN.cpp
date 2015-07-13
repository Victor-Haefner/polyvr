#include "VRPN.h"
#include "core/utils/VROptions.h"
#include "core/gui/VRGuiManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/toString.h"
#include <libxml++/nodes/element.h>
#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>
#include <vrpn_Dial.h>
#include <vrpn_Text.h>
#include <boost/bind.hpp>
#include "core/utils/VRFunction.h"
#include "core/setup/devices/VRDevice.h"
#include "core/objects/VRTransform.h"
#include "core/utils/VRStorage_template.h"

#include <OpenSG/OSGQuaternion.h>

OSG_BEGIN_NAMESPACE


void VRPN_CALLBACK handle_tracker(void* data, const vrpn_TRACKERCB tracker ) {
    VRPN_device* dev = (VRPN_device*)data;
    VRTransform* obj = dev->editBeacon();
    if (obj == 0) return;

    Vec3f ra = dev->rotation_axis;
    Vec3f ta = dev->translate_axis;
    Vec3f sra, sta;
    for (int i=0; i<3; i++) {
        sra[i] = ra[i]<0 ? -1 : 1; ra[i] = abs(ra[i]);
        sta[i] = ta[i]<0 ? -1 : 1; ta[i] = abs(ta[i]);
    }

    //rotation
    Quaternion q(sra[0]*tracker.quat[(int)ra[0]], sra[1]*tracker.quat[(int)ra[1]], sra[2]*tracker.quat[(int)ra[2]], tracker.quat[3]);
    //Quaternion q2(1,0,0,0.5*3.14);
    //q += q2;
    //Quaternion q(tracker.quat[0], -tracker.quat[2], -tracker.quat[1], tracker.quat[3]);
    Matrix m;
    m.setRotate(q);

    //position
    float s = dev->scale;
    Vec3f pos = dev->offset + Vec3f(sta[0]*tracker.pos[(int)ta[0]]*s, sta[1]*tracker.pos[(int)ta[1]]*s, sta[2]*tracker.pos[(int)ta[2]]*s);
    for (int i=0; i<3; i++) m[3][i] = pos[i];

    obj->setMatrix(m);
}

void VRPN_CALLBACK handle_button(void* data, const vrpn_BUTTONCB button ) {
    VRPN_device* dev = (VRPN_device*)data;
    dev->change_button(button.button, button.state);
}

void VRPN_CALLBACK handle_analog(void* data, const vrpn_ANALOGCB analog ) {
    VRPN_device* dev = (VRPN_device*)data;
    for (int i=0; i<analog.num_channel; i++) dev->change_slider(i+20, analog.channel[i]);
}

VRPN_device::VRPN_device() : VRDevice("vrpn_device") {
    store("address", &address);
    store("offset", &offset);
    store("scale", &scale);
    store("ID", &ID);
    store("taxis", &translate_axis);
    store("raxis", &rotation_axis);

    enableAvatar("cone");
    enableAvatar("ray");

    VRSetupManager::getCurrent()->addDevice(this);
}

void VRPN_device::setAddress(string addr) {
    address = addr;
    if (tracker) delete tracker; tracker = 0;
    if (button) delete button; button = 0;
    if (analog) delete analog; analog = 0;
    if (dial) delete dial; dial = 0;
    if (text) delete text; text = 0;

    if (address == "") return;

    tracker = new vrpn_Tracker_Remote( addr.c_str() ); // TODO: check which of these is supported from the remote
    button = new vrpn_Button_Remote( addr.c_str() );
    analog = new vrpn_Analog_Remote( addr.c_str() );
    dial = new vrpn_Dial_Remote( addr.c_str() );
    text = new vrpn_Text_Receiver( addr.c_str() );

    tracker->register_change_handler( this, handle_tracker ); // TODO: add other handlers
    button->register_change_handler( this, handle_button ); // TODO: add other handlers
    analog->register_change_handler( this, handle_analog ); // TODO: add other handlers
    initialized = true;
}

void VRPN_device::setTranslationAxis(Vec3f v) { translate_axis = v; }
void VRPN_device::setRotationAxis(Vec3f v) { rotation_axis = v; }

void VRPN_device::loop() {
    if (!initialized) setAddress(address);

    if (tracker) tracker->mainloop();
    if (button) button->mainloop();
    if (analog) analog->mainloop();
    if (dial) dial->mainloop();
    if (text) text->mainloop();
}


VRPN::VRPN() {
    //auto update_cb = new VRFunction<VRThread*>("VRPN_update", boost::bind(&VRPN::update_t, this, _1));
    //threadID = VRSceneManager::get()->initThread(update_cb, "VRPN", true);

    auto update_cb = new VRFunction<int>("VRPN_update", boost::bind(&VRPN::update, this));
    VRSceneManager::get()->addUpdateFkt(update_cb);

    vrpn_System_TextPrinter.set_ostream_to_use(NULL);

    storeMap("Tracker", &devices);
    store("active", &active);
    store("port", &port);
}

VRPN::~VRPN() {
    VRSceneManager::get()->stopThread(threadID);
}

void VRPN::update_t(VRThread* thread) {}
void VRPN::update() {
    if (!active) return;

    if (verbose) VRGuiManager::get()->printInfo("vrpn verbooooose\n");

    for (auto tr : devices) tr.second->loop();
}

void VRPN::setVRPNVerbose(bool b) { verbose = b; }

void VRPN::addVRPNTracker(int ID, string addr, Vec3f offset, float scale) {
    while(devices.count(ID)) ID++;

    VRPN_device* t = new VRPN_device();
    t->ID = ID;
    t->offset = offset;
    t->scale = scale;
    t->setAddress(addr);

    devices[ID] = t;
}

void VRPN::delVRPNTracker(VRPN_device* t) {
    devices.erase(t->ID);
    delete t;
}

vector<int> VRPN::getVRPNTrackerIDs() {
    vector<int> IDs;
    for (auto t : devices) IDs.push_back(t.first);
    return IDs;
}

VRPN_device* VRPN::getVRPNTracker(int ID) {
    if (devices.count(ID)) return devices[ID];
    else return 0;
}

void VRPN::changeVRPNDeviceName(VRPN_device* dev, string name) {
    dev->setName(name);
    cout << "set name " << name << endl;
}

void VRPN::setVRPNActive(bool b) { active = b; }
bool VRPN::getVRPNActive() { return active; }

void VRPN::setVRPNPort(int p) { port = p; }
int VRPN::getVRPNPort() { return port; }


// ----------------------------- test server -----------------------------------

// VRPN Server tutorial
// by Sebastien Kuntz, for the VR Geeks (http://www.vrgeeks.org)
// August 2011

#include <stdio.h>
#include <math.h>

#include "vrpn_Text.h"
#include "vrpn_Tracker.h"
#include "vrpn_Analog.h"
#include "vrpn_Button.h"
#include "vrpn_Connection.h"

#include <iostream>
using namespace std;

/////////////////////// TRACKER /////////////////////////////

// your tracker class must inherit from the vrpn_Tracker class
class myTracker : public vrpn_Tracker
{
public:
    myTracker( vrpn_Connection *c = 0 );
    virtual ~myTracker() {};

    virtual void mainloop();

protected:
    struct timeval _timestamp;
};

myTracker::myTracker( vrpn_Connection *c /*= 0 */ ) :
    vrpn_Tracker( "Tracker0", c )
{
}

void
myTracker::mainloop()
{
    vrpn_gettimeofday(&_timestamp, NULL);

    vrpn_Tracker::timestamp = _timestamp;

    // We will just put a fake data in the position of our tracker
    static float angle = 0; angle += 0.001f;

    // the pos array contains the position value of the tracker
    // XXX Set your values here
    pos[0] = sinf( angle );
    pos[1] = 0.0f;
    pos[2] = 0.0f;

    // the d_quat array contains the orientation value of the tracker, stored as a quaternion
    // XXX Set your values here
    d_quat[0] = 0.0f;
    d_quat[1] = 0.0f;
    d_quat[2] = 0.0f;
    d_quat[3] = 1.0f;

    char msgbuf[1000];

    d_sensor = 0;

    int  len = vrpn_Tracker::encode_to(msgbuf);

    if (d_connection->pack_message(len, _timestamp, position_m_id, d_sender_id, msgbuf,
        vrpn_CONNECTION_LOW_LATENCY))
    {
        fprintf(stderr,"can't write message: tossing\n");
    }

    server_mainloop();
}

/////////////////////// ANALOG /////////////////////////////

// your analog class must inherin from the vrpn_Analog class
class myAnalog : public vrpn_Analog
{
public:
    myAnalog( vrpn_Connection *c = 0 );
    virtual ~myAnalog() {};

    virtual void mainloop();

protected:
    struct timeval _timestamp;
};


myAnalog::myAnalog( vrpn_Connection *c /*= 0 */ ) :
    vrpn_Analog( "Analog0", c )
{
    vrpn_Analog::num_channel = 10;

    vrpn_uint32    i;

    for (i = 0; i < (vrpn_uint32)vrpn_Analog::num_channel; i++) {
        vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
    }
}

void myAnalog::mainloop() {
    vrpn_gettimeofday(&_timestamp, NULL);
    vrpn_Analog::timestamp = _timestamp;

    // forcing values to change otherwise vrpn doesn't report the changes
    static float f = 0; f+=0.001;

    for(int i=0; i<vrpn_Analog::num_channel;i++) {
        channel[i] = i / 10.f + f;
    }

    // Send any changes out over the connection.
    vrpn_Analog::report_changes();

    server_mainloop();
}

/////////////////////// BUTTON /////////////////////////////

// your button class must inherit from the vrpn_Button class
class myButton : public vrpn_Button {
    public:
        myButton( vrpn_Connection *c = 0 );
        virtual ~myButton() {};

        virtual void mainloop();

    protected:
        struct timeval _timestamp;
};


myButton::myButton( vrpn_Connection* c ) : vrpn_Button( "Button0", c ) {
    // Setting the number of buttons to 10
    vrpn_Button::num_buttons = 10;
    vrpn_uint32 i;

    // initializing all buttons to false
    for (i = 0; i < (vrpn_uint32)vrpn_Button::num_buttons; i++) {
        vrpn_Button::buttons[i] = vrpn_Button::lastbuttons[i] = 0;
    }
}

void myButton::mainloop() {
    vrpn_gettimeofday(&_timestamp, NULL);
    vrpn_Button::timestamp = _timestamp;

    // forcing values to change otherwise vrpn doesn't report the changes
    for (int i=0; i<vrpn_Button::num_buttons; i++) buttons[i] = (i+buttons[i])%2;

    // Send any changes out over the connection.
    vrpn_Button::report_changes();
    server_mainloop();
}

////////////// MAIN ///////////////////


vrpn_Connection_IP* m_Connection;

// Creating the tracker
myTracker* serverTracker;
myAnalog*  serverAnalog;
myButton*  serverButton;

void vrpn_test_server_main() {
    serverTracker->mainloop();
    serverAnalog->mainloop();
    serverButton->mainloop();
    m_Connection->mainloop();
    //vrpn_SleepMsecs(1);
}

void VRPN::stopVRPNTestServer() {
    if (testServer == 0) return;
    VRSceneManager::get()->dropUpdateFkt(testServer);
    delete testServer;
    testServer = 0;
    delete serverButton;
    delete serverAnalog;
    delete serverTracker;
    delete m_Connection;
}

void VRPN::startVRPNTestServer() {
    if (testServer != 0) return;
    // Creating the network server
    m_Connection = new vrpn_Connection_IP();

    // Creating the tracker
    serverTracker = new myTracker(m_Connection );
    serverAnalog  = new myAnalog(m_Connection );
    serverButton  = new myButton(m_Connection );

    cout << "Created VRPN server." << endl;

    testServer = new VRFunction<int>("VRPN_test_server", boost::bind(vrpn_test_server_main));
    VRSceneManager::get()->addUpdateFkt(testServer);
}

OSG_END_NAMESPACE
