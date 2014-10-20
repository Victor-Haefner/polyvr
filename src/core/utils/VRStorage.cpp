#include "VRStorage.h"
#include "toString.h"
#include <libxml++/nodes/element.h>
#include <boost/bind.hpp>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRStorage::VRStorage() {
    ;
}

void VRStorage::save(xmlpp::Element* e) {
    if (e == 0) return;
    for (itr = storage.begin(); itr != storage.end(); itr++) {
        itr->second.f2(e);
    }
}

void VRStorage::load(xmlpp::Element* e) {
    if (e == 0) return;
    for (itr = storage.begin(); itr != storage.end(); itr++) {
        itr->second.f1(e);
    }
}

OSG_END_NAMESPACE;
