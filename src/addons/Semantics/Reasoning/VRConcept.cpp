#include "VRConcept.h"

#include <iostream>

VRConcept::VRConcept(string name) {
    this->name = name;
}

void VRConcept::append(VRConcept* c) { children[c->ID] = c; c->parent = this; }
void VRConcept::addProperty(VRProperty* p) { properties[p->ID] = p; }

VRConcept* VRConcept::append(string name) {
    auto c = new VRConcept(name);
    append(c);
    return c;
}

VRProperty* VRConcept::addProperty(string name, string type) {
    auto p = new VRProperty(name, type);
    addProperty(p);
    return p;
}

VRProperty* VRConcept::getProperty(int ID) {
    for (auto p : properties) if (p.second->ID == ID) return p.second;
    return 0;
}

VRProperty* VRConcept::getProperty(string type) {
    for (auto p : properties) if (p.second->type == type) return p.second;
    return 0;
}

int VRConcept::getPropertyID(string name) {
    for (auto p : properties) if (p.second->name == name) return p.second->ID;
    return -1;
}

bool VRConcept::is_a(string concept) {
    VRConcept* c = this;
    while (c) {
        if (c->name == concept) return true;
        c = c->parent;
    }
    return false;
}
