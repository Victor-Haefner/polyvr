#include "VRProperty.h"

#include <iostream>

VRProperty::VRProperty(string name, string type) {
    this->name = name;
    this->type = type;
}

VRPropertyPtr VRProperty::create(string name, string type) { return VRPropertyPtr(new VRProperty(name, type)); }

void VRProperty::setType(string type) { this->type = type; }

string VRProperty::toString() {
    string res;
    res += " prop "+name+" ("+type+")\n";
    return res;
}

VRPropertyPtr VRProperty::copy() {
    auto p = create(name, type);
    p->value = value;
    return p;
}
