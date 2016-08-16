#include "VRProperty.h"

#include <iostream>

using namespace OSG;

VRProperty::VRProperty(string name, string type) {
    setStorageType("Property");
    setNameSpace("property");
    setUniqueName(false);
    setName(name);
    this->type = type;
}

VRPropertyPtr VRProperty::create(string name, string type) { return VRPropertyPtr(new VRProperty(name, type)); }

void VRProperty::setType(string type) { this->type = type; }

string VRProperty::toString() {
    string res;
    res += " prop "+name+" = "+value+" ("+type+")\n";
    return res;
}

VRPropertyPtr VRProperty::copy() {
    auto p = create(name, type);
    p->value = value;
    return p;
}
