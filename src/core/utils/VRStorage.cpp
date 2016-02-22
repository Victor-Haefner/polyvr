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

void VRStorage::regStorageUpdateFkt(VRUpdatePtr u) { f_update = u; }

void VRStorage::save(xmlpp::Element* e) {
    if (e == 0) return;
    for (auto s : storage) (*s.second.f2)(e);
}

void VRStorage::saveUnder(xmlpp::Element* e) {
    if (e == 0) return;
    e = e->add_child("node");// todo: define tag
    save(e);
}

void VRStorage::load(xmlpp::Element* e) {
    if (e == 0) return;
    for (auto s : storage) (*s.second.f1)(e);
    if (f_update) (*f_update)(0);
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
