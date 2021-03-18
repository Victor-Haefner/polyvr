#include "VRProperty.h"
#include "core/utils/toString.h"

#include <iostream>
#include <algorithm>

using namespace OSG;

VRProperty::VRProperty(string name, string t) {
    setStorageType("Property");
    auto ns = setNameSpace("VRProperty");
    ns->filterNameChars(".,",'_'); // filter path and math characters
    ns->setSeparator('_');
    ns->setUniqueNames(false);
    setName(name);
    type = t;

    store("type", &type);
    store("value", &value);
}

VRPropertyPtr VRProperty::create(string name, string type) { return VRPropertyPtr(new VRProperty(name, type)); }

void VRProperty::setType(string type) { this->type = type; }
string VRProperty::getType() { return type; }
string VRProperty::getValue() { return value; }

void VRProperty::setValue(string value) {
    if (!isNumber(value)) for (char c : string(".,")) replace(value.begin(), value.end(),c,'_');
    this->value = value;
}

string VRProperty::toString() {
    string res;
    res += " prop "+name+" = "+value+" ("+type+")";
    for (auto p : parents) res += " -> " + p;
    res += "\n"; // <- why?
    return res;
}

VRPropertyPtr VRProperty::copy() {
    auto p = create(base_name, type);
    p->setValue( value );
    return p;
}
