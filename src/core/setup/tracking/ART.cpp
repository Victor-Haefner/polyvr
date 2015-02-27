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

OSG_BEGIN_NAMESPACE;
using namespace std;

ART_device::ART_device() {
    scale = 1;
    ent = 0;
    ID = 0;
    type = 0;
    dev = 0;
    setName("ARTDevice");
}

//hohlt die Orientierung des getrackten Objektes
template<typename type>
void ART::getRotation(type t, ART_device* dev, Matrix& m) {
    if (t.quality > 0) {
        m[0] = Vec4f(t.rot[0], t.rot[2], -t.rot[1], 0);//LESC
        m[1] = Vec4f(t.rot[6], t.rot[8], -t.rot[7], 0);
        m[2] = Vec4f(-t.rot[3], -t.rot[5], t.rot[4], 0);
    }
}

//hohlt die Position des getrackten Objektes
template<typename type>
void ART::getPosition(type t, ART_device* dev, Matrix& m) {
    if (t.quality > 0){
        float k = 0.001*dev->scale;
        m[3] = Vec4f(t.loc[0]*k, t.loc[2]*k, -t.loc[1]*k, m[3][3]) + Vec4f(dev->offset) + Vec4f(offset);//LESC
        //m[3] = Vec4f(t.loc[0]/1000*Esys, t.loc[1]/1000*Esys, t.loc[2]/1000*Esys, m[3][3]) + offset;
    }
}

void ART::checkIncomming() {
    Matrix m;

    for (itr = devices.begin(); itr != devices.end(); itr++) {
        ART_device* dev = itr->second;

        VRTransform* ent = dev->ent;
        if (ent == 0) return;

        dtrack_flystick_type fly;

        ent->getMatrix(m);
        switch(dev->type) {
            case 0://body
                getRotation(dtrack->get_body(dev->ID), dev, m);
                getPosition(dtrack->get_body(dev->ID), dev, m);
                break;
            case 1://flystick
                fly = dtrack->get_flystick(dev->ID);
                getRotation(fly, dev, m);
                getPosition(fly, dev, m);
                if (dev->dev) dev->dev->update(fly.num_button, fly.button, fly.num_joystick, fly.joystick);
                break;
            case 2://head (not used)
                //getRotation(dtrack->get_body(ID),m);//auch in OSG 2 flackern -> eher ein ART Problem! -> netzwerk? -> UDP?
                getPosition(dtrack->get_body(dev->ID), dev, m);
                break;
        }
        ent->setMatrix(m);
    }
}

//update thread
void ART::update() {
    if (dtrack == 0) dtrack = new DTrack(port, 0, 0, 20000, 10000);
    if (!active or dtrack == 0 or devices.size() == 0) return;

    if (dtrack->receive()) {// 60 fps
        checkIncomming();
    } else {
        if(dtrack->timeout())       cout << "--- ART: timeout while waiting for udp data" << endl;
        if(dtrack->udperror())      cout << "--- ART: error while receiving udp data" << endl;
        if(dtrack->parseerror())    cout << "--- ART: error while parsing udp data" << endl;
    }
}

ART::ART() {
    dtrack = 0;
    active = false;
    port = 5000;

    //VRSceneManager::get()->initThread(getARTUpdateFkt(), "ART", true);
    VRSceneManager::get()->addUpdateFkt(getARTUpdateFkt());
}

ART::~ART() {
    //VRSceneManager::get()->initThread(getARTUpdateFkt(), "ART", true);
    VRSceneManager::get()->dropUpdateFkt(getARTUpdateFkt());
}

ART_device* ART::addARTDevice(VRTransform* trans) {
    ART_device* t = addARTDevice();
    t->ent = trans;
    if (t->ent) VRSetupManager::getCurrent()->addObject(t->ent);
    return t;
}

ART_device* ART::addARTDevice(VRFlystick* dev) {
    ART_device* t = new ART_device();
    t->dev = dev;
    t->type = 1;
    if (dev) t->ent = t->dev->getBeacon();
    devices[t->getName()] = t;
    if (t->ent) VRSetupManager::getCurrent()->addObject(t->ent);
    return t;
}

vector<string> ART::getARTDevices() {
    vector<string> devs;
    for (itr = devices.begin(); itr != devices.end(); itr++) {
        devs.push_back(itr->first);
    }
    return devs;
}
ART_device* ART::getARTDevice(string s) { if (devices.count(s)) return devices[s]; else return 0;  }

VRFunction<int>* ART::getARTUpdateFkt() {
    return new VRFunction<int>("ART_update", boost::bind(&ART::update, this));
}

void ART::setARTActive(bool b) { active = b; }
bool ART::getARTActive() { return active; }

void ART::setARTPort(int port) {
    if (this->port == port) return;
    this->port = port;

    if (dtrack != 0) delete dtrack;
    dtrack = new DTrack(port);
    if (!dtrack->valid()) {
        cout << "DTrack init error" << endl;
        delete dtrack;
        port = -1;
        dtrack = 0;
    }
}
int ART::getARTPort() { return port; }

void ART::setARTOffset(Vec3f o) { offset = o; }
Vec3f ART::getARTOffset() { return offset; }

void ART::save(xmlpp::Element* node) {
    node->set_attribute("active", toString(active));
    node->set_attribute("port", toString(port));
    node->set_attribute("offset", toString(offset));

    xmlpp::Element* sn;
    for (itr = devices.begin(); itr != devices.end(); itr++) {
        ART_device* ad = itr->second;
        if (ad->type != 0) continue; // ignore flysticks, they are loaded elsewhere && creater their own tracker

        sn = node->add_child("Tracker");
        sn->set_attribute("scale", toString(ad->scale));
        sn->set_attribute("ID", toString(ad->ID));
        sn->set_attribute("type", toString(ad->type));
        ad->saveName(sn);
    }
}

void ART::load(xmlpp::Element* node) {
    if (node->get_attribute("active") == 0) return;
    active = toBool( node->get_attribute("active")->get_value() );
    port = toInt( node->get_attribute("port")->get_value() );
    setARTPort(port);

    if (node->get_attribute("offset")) setARTOffset(toVec3f( node->get_attribute("offset")->get_value() ));

    xmlpp::Node::NodeList nl = node->get_children();
    xmlpp::Node::NodeList::iterator itr;
    for (itr = nl.begin(); itr != nl.end(); itr++) {
        xmlpp::Node* n = *itr;

        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        string name = el->get_name();
        if (name != "Tracker") continue;

        name = el->get_attribute("base_name")->get_value();
        VRTransform* tr = new VRTransform(name);
        tr->setFrom(Vec3f(0,1.6,0));
        ART_device* t = addARTDevice(tr);

        t->setName(name);
        t->scale = toFloat( el->get_attribute("scale")->get_value() );
        t->ID = toInt( el->get_attribute("ID")->get_value() );
        t->type = toInt( el->get_attribute("type")->get_value() );
    }
}

OSG_END_NAMESPACE
