#include "VRStorage.h"
#include "toString.h"
#include "VRFunction.h"
#include "core/scene/VRSceneManager.h"
#include <libxml++/nodes/element.h>
#include <boost/bind.hpp>

OSG_BEGIN_NAMESPACE;
using namespace std;

map<string, VRStorageFactoryCbPtr> VRStorage::factory = map<string, VRStorageFactoryCbPtr>();

VRStorage::VRStorage() {
    store("persistency", &persistency);
}

void VRStorage::setPersistency(int p) { persistency = p; }
int VRStorage::getPersistency() { return persistency; }
void VRStorage::regStorageSetupFkt(VRUpdatePtr u) { f_setup.push_back(u); }
void VRStorage::regStorageSetupAfterFkt(VRUpdatePtr u) { f_setup_after.push_back(u); }
void VRStorage::setStorageType(string t) { type = t; }

void VRStorage::save(xmlpp::Element* e, int p) {
    if (type == "Script") cout << "VRStorage::save " << e << endl;
    if (e == 0) return;
    if (persistency <= p) return;
    for (auto s : storage) (*s.second.f2)(e);
}

xmlpp::Element* VRStorage::saveUnder(xmlpp::Element* e, int p) {
    if (type == "Script") cout << "VRStorage::saveUnder " << e << endl;
    if (e == 0) return 0;
    if (persistency <= p) return 0;
    e = e->add_child(type);
    save(e, p);
    return e;
}

void VRStorage::load(xmlpp::Element* e) {
    if (e == 0) return;
    for (auto s : storage) (*s.second.f1)(e);
    for (auto f : f_setup) (*f)(0);
    for (auto f : f_setup_after) VRSceneManager::get()->queueJob(f);
    f_setup_after.clear();
}

void VRStorage::loadChildFrom(xmlpp::Element* e) { load( getChild(e, type) ); }

int VRStorage::getPersistency(xmlpp::Element* e) {
    if (!e->get_attribute("persistency")) return 0;
    return toInt( e->get_attribute("persistency")->get_value() );
}

VRStoragePtr VRStorage::createFromStore(xmlpp::Element* e) {
    if (!e->get_attribute("type")) return 0;
    string type = e->get_attribute("type")->get_value();
    //cout << "VRStorage::createFromStore " << type << " " << factory.count(type) << endl;
    if (!factory.count(type)) return 0;

    VRStoragePtr res;
    (*factory[type])(res);
    return res;
}

xmlpp::Element* VRStorage::getChild(xmlpp::Element* e, string c) {
    if (e == 0) return 0;
    for (auto n : e->get_children()) {
        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;
        if (el->get_name() == c) return el;
    }
    return 0;
}

OSG_END_NAMESPACE;
