#include "VRConcept.h"
#include "VRProperty.h"

#include <iostream>

VRConcept::VRConcept(string name) {
    this->name = name;
}

VRConceptPtr VRConcept::create(string name) { return VRConceptPtr(new VRConcept(name)); }

void VRConcept::append(VRConceptPtr c) { children[c->ID] = c; c->parent = shared_from_this(); }
void VRConcept::addProperty(VRPropertyPtr p) { properties[p->ID] = p; }

VRConceptPtr VRConcept::append(string name) {
    auto c = VRConcept::create(name);
    append(c);
    return c;
}

VRPropertyPtr VRConcept::addProperty(string name, string type) {
    auto p = VRProperty::create(name, type);
    addProperty(p);
    return p;
}

VRPropertyPtr VRConcept::getProperty(int ID) {
    for (auto p : properties) if (p.second->ID == ID) return p.second;
    return 0;
}

VRPropertyPtr VRConcept::getProperty(string type) {
    for (auto p : properties) if (p.second->type == type) return p.second;
    return 0;
}

vector<VRPropertyPtr> VRConcept::getProperties() {
    vector<VRPropertyPtr> res;
    for (auto p : properties) res.push_back(p.second);
    return res;
}

int VRConcept::getPropertyID(string name) {
    for (auto p : properties) if (p.second->name == name) return p.second->ID;
    return -1;
}

bool VRConcept::is_a(string concept) {
    VRConceptPtr c = shared_from_this();
    while (c) {
        if (c->name == concept) return true;
        c = c->parent.lock();
    }
    return false;
}

string VRConcept::toString(string indent) {
    string res = indent+"concept: "+name+"\n";
    for (auto p : properties) res += indent+p.second->toString();
    for (auto c : children) res += c.second->toString(indent+"  ");
    return res;
}
