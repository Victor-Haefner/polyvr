#include "VRProperty.h"

#include <iostream>

using namespace OSG;

VRProperty::VRProperty(string name, string t) {
    setStorageType("Property");
    setNameSpace("property");
    setSeparator('_');
    setUniqueName(false);
    setName(name);
    type = t;

    store("type", &type);
    store("value", &value);
}

VRPropertyPtr VRProperty::create(string name, string type) { return VRPropertyPtr(new VRProperty(name, type)); }

void VRProperty::setType(string type) { this->type = type; }

string VRProperty::toString() {
    string res;
    res += " prop "+name+" = "+value+" ("+type+")\n";
    return res;
}

VRPropertyPtr VRProperty::copy() {
    auto p = create(base_name, type);
    p->value = value;
    return p;
}
