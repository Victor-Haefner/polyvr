#include "VRStorage.h"
#include "toString.h"
#include "VRFunction.h"
#include <libxml++/nodes/element.h>
#include <boost/bind.hpp>

OSG_BEGIN_NAMESPACE;
using namespace std;

map<string, VRStorageFactoryCbPtr> VRStorage::factory = map<string, VRStorageFactoryCbPtr>();

VRStorage::VRStorage() {
    ;
}

void VRStorage::setPersistency(int p) { persistency = p; }
int VRStorage::getPersistency() { return persistency; }
void VRStorage::regStorageUpdateFkt(VRUpdatePtr u) { f_update.push_back(u); }
void VRStorage::setStorageType(string t) { type = t; }

void VRStorage::save(xmlpp::Element* e, int p) {
    if (e == 0) return;
    if (persistency <= p) return;
    for (auto s : storage) (*s.second.f2)(e);
}

void VRStorage::saveUnder(xmlpp::Element* e, int p) {
    if (e == 0) return;
    if (persistency <= p) return;
    e = e->add_child(type); // todo: define tag
    save(e, p);
}

void VRStorage::load(xmlpp::Element* e) {
    if (e == 0) return;
    for (auto s : storage) (*s.second.f1)(e);
    for (auto f : f_update) (*f)(0);
}

VRStoragePtr VRStorage::createFromStore(xmlpp::Element* e) {
    if (!e->get_attribute("type")) return 0;
    string type = e->get_attribute("type")->get_value();
    if (!factory.count(type)) return 0;

    VRStoragePtr res;
    (*factory[type])(res);
    return res;
}

OSG_END_NAMESPACE;
