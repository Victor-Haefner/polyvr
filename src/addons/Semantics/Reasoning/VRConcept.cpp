#include "VRConcept.h"
#include "VRProperty.h"

#include <iostream>

VRConcept::VRConcept(string name) {
    this->name = name;
}

VRConceptPtr VRConcept::create(string name) {
    return VRConceptPtr(new VRConcept(name));
}

void VRConcept::append(VRConceptPtr c) { children[c->ID] = c; c->parent = shared_from_this(); }
void VRConcept::addProperty(VRPropertyPtr p) { properties[p->ID] = p; }
void VRConcept::addAnnotation(VRPropertyPtr p) { annotations[p->ID] = p; }

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
    for (auto p : getProperties()) if (p->ID == ID) return p;
    cout << "Warning: property with ID " << ID << " not found" << endl;
    return 0;
}

VRPropertyPtr VRConcept::getProperty(string name) {
    for (auto p : getProperties()) if (p->name == name) return p;
    for (auto p : annotations) if (p.second->name == name) return p.second;
    cout << "Warning: property " << name << " of concept " << this->name << " not found!" << endl;
    return 0;
}

void VRConcept::getProperties(map<string, VRPropertyPtr>& res) {
    for (auto p : properties) res[p.second->name] = p.second;
    if (auto p = parent.lock()) p->getProperties(res);
}

vector<VRPropertyPtr> VRConcept::getProperties(string type) {
    vector<VRPropertyPtr> res;
    for (auto p : getProperties()) if (p->type == type) res.push_back(p);
    if (res.size() == 0) cout << "Warning: no properties of type " << type << " found!" << endl;
    if (res.size() > 1) cout << "Warning: multiple properties of type " << type << " found!" << endl;
    return res;
}

vector<VRPropertyPtr> VRConcept::getProperties() {
    vector<VRPropertyPtr> res;
    map<string, VRPropertyPtr> tmp;
    if (auto p = parent.lock()) p->getProperties(tmp);
    for (auto p : properties) tmp[p.second->name] = p.second;
    for (auto p : tmp) res.push_back(p.second);
    return res;
}

int VRConcept::getPropertyID(string name) {
    for (auto p : getProperties()) if (p->name == name) return p->ID;
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
    for (auto a : annotations) res += indent+a.second->toString();
    for (auto p : getProperties()) res += indent+p->toString();
    for (auto c : children) res += c.second->toString(indent+"  ");
    return res;
}
