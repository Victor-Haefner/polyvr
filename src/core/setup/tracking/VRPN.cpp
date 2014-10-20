#include "VRPN.h"
#include "core/utils/VROptions.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/toString.h"
#include <libxml++/nodes/element.h>
#include <vrpn_Tracker.h>
#include <boost/bind.hpp>
#include "core/utils/VRFunction.h"
#include "core/objects/VRTransform.h"

OSG_BEGIN_NAMESPACE


VRPN_tracker::VRPN_tracker() {
    scale = 1;
    ent = 0;
    vrpnt = 0;
    vrpnc = 0;
    ID = 0;
}

void VRPN_tracker::setTracker(string t) {
    tracker = t;
    if (vrpnt) delete vrpnt;
    vrpnt = 0;
}

void VRPN_CALLBACK handle_tracker(void* data, const vrpn_TRACKERCB t ) { // TODO
    return;
    VRPN_tracker* tracker = (VRPN_tracker*)data;
    cout << "Tracker '" << t.sensor << "' : " << t.pos[0] << "," <<  t.pos[1] << "," << t.pos[2] << endl;

    VRTransform* obj = tracker->ent;
    if (obj == 0) return;

    //Matrix m;
    //obj->getMatrix(m);
    //rotation
    //getRotation(t->get_body(ID),m);
    //position
    float s = tracker->scale;
    Vec3f pos = Vec3f(-t.pos[0]*s, -t.pos[1]*s, t.pos[2]*s);
    pos += tracker->offset;
    //m[3] = Vec4f(-t.pos[0]*Esys, -t.pos[1]*Esys + groundD, t.pos[2]*Esys, m[3][3]);
    //m[3] = Vec4f(-t.pos[0], -t.pos[1], 0.8, m[3][3]);
    //m[3] = Vec4f(0,0,1, m[3][3]);
    //obj->setMatrix(m);
    obj->setPose(pos, pos+Vec3f(0,0,1), Vec3f(0,1,0));
}

VRPN::VRPN() {
    auto update_cb = new VRFunction<VRThread*>("VRPN_update", boost::bind(&VRPN::update, this, _1));
    threadID = VRSceneManager::get()->initThread(update_cb, "VRPN", true);
}

VRPN::~VRPN() {
    cout << "\n~VRPN()" << endl;
    VRSceneManager::get()->stopThread(threadID);
}

void VRPN::update(VRThread* thread) {
    timeval* timeout = new timeval();
    timeout->tv_sec = 0;
    timeout->tv_usec = 10;

    static int state = 0;

    //vrpn_System_TextPrinter.set_min_level_to_print(vrpn_TEXT_ERROR, 0);
    //vrpn_System_TextPrinter.set_min_level_to_print(vrpn_TEXT_NORMAL, 0);
    //vrpn_System_TextPrinter.set_min_level_to_print(vrpn_TEXT_WARNING, 0);
    //vrpn_System_TextPrinter.set_ostream_to_use(NULL);

    for (auto tr : tracker) {
        VRPN_tracker* t = tr.second;

        if (t->vrpnt == 0) {
            t->vrpnt = new vrpn_Tracker_Remote( t->tracker.c_str() );
            t->vrpnt->register_change_handler( t, handle_tracker );
        }

        if (state == 0) {
            state = 1;
            if (VRGlobals::get()->CURRENT_FRAME%5 == 0) t->vrpnt->mainloop();
            state = 0;
        }

    }

    delete timeout;
}

void VRPN::addVRPNTracker(int ID, string addr, Vec3f offset, float scale) {
    while(tracker.count(ID)) ID++;

    VRPN_tracker* t = new VRPN_tracker();
    t->ent = new VRTransform("VRPN_tracker" + toString(ID));
    t->tracker = addr;
    t->ID = ID;
    t->offset = offset;
    t->scale = scale;
    VRSetupManager::get()->getCurrent()->addObject(t->ent);

    tracker[ID] = t;
}


vector<int> VRPN::getVRPNTrackerIDs() {
    vector<int> IDs;
    for (auto t : tracker) IDs.push_back(t.first);
    return IDs;
}

VRPN_tracker* VRPN::getVRPNTracker(int ID) {
    if (tracker.count(ID)) return tracker[ID];
    else return 0;
}

void VRPN::save(xmlpp::Element* node) {
    xmlpp::Element* tn;
    for (auto tr : tracker) {
        VRPN_tracker* t = tr.second;
        tn = node->add_child("Tracker");
        tn->set_attribute("address", t->tracker);
        tn->set_attribute("offset", toString(t->offset));
        tn->set_attribute("scale", toString(t->scale));
        tn->set_attribute("ID", toString(t->ID));
    }
}

void VRPN::load(xmlpp::Element* node) {
    xmlpp::Node::NodeList nl = node->get_children();
    xmlpp::Node::NodeList::iterator itr;
    for (itr = nl.begin(); itr != nl.end(); itr++) {
        xmlpp::Node* n = *itr;

        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        int ID = toInt(el->get_attribute("ID")->get_value());
        Vec3f off = toVec3f(el->get_attribute("offset")->get_value());
        float s = toFloat(el->get_attribute("scale")->get_value());

        //cout << "\nADD TRACKER " << ID << " " << s << " " << off << flush;

        addVRPNTracker(ID, el->get_attribute("address")->get_value(), off, s);
    }
}

OSG_END_NAMESPACE
