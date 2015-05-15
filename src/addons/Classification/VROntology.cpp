#include "VROntology.h"

#include <iostream>

int guid() {
    static int id = 0;
    id++;
    return id;
}

VRNamedID::VRNamedID() {
    ID = guid();
}

VRProperty::VRProperty(string name, string type) {
    this->name = name;
    this->type = type;
}

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

VRTaxonomy::VRTaxonomy() {
    thing = new VRConcept("Thing");
}

VRConcept* VRTaxonomy::get(string name, VRConcept* p) {
    if (p == 0) p = thing;
    if (p->name == name) return p;
    VRConcept* c = 0;
    for (auto ci : p->children) {
        c = get(name, ci.second);
        if (c) return c;
    }
    return c;
}

VROntology::VROntology() {
    taxonomy = new VRTaxonomy();
}

void VROntology::merge(VROntology* o) {
    for (auto c : o->taxonomy->thing->children)
        taxonomy->thing->append(c.second);
}

int VRConcept::getPropertyID(string name) {
    for (auto p : properties) if (p.second->name == name) return p.second->ID;
    return -1;
}

void VROntologyInstance::set(string name, string value) {
    int id = concept->getPropertyID(name);
    if (id < 0) return;

    if (!properties.count(id)) { add(name, value); return; }

    properties[id][0] = value;
}

void VROntologyInstance::add(string name, string value) {
    int id = concept->getPropertyID(name);
    if (id < 0) return;

    if (!properties.count(id)) properties[id] = vector<string>();

    properties[id].push_back(value);
}

VROntologyInstance::VROntologyInstance(string name, VRConcept* c) {
    this->name = name;
    concept = c;
}

string VROntologyInstance::toString() {
    string data = "Instance " + name + " of type " + concept->name;
    data += " with properties:";
    for (auto p : properties) {
        for (auto sp : p.second) {
            data += " "+concept->properties[p.first]->name+"="+sp;
        }
    }
    return data;
}

bool VRConcept::is_a(string concept) {
    VRConcept* c = this;
    while (c) {
        if (c->name == concept) return true;
        c = c->parent;
    }
    return false;
}

VROntologyInstance* VROntology::addInstance(string concept, string name) {
    auto c = taxonomy->get(concept);
    auto i = new VROntologyInstance(name, c);
    instances[i->ID] = i;
    return i;
}

vector<VROntologyInstance*> VROntology::getInstances(string concept) {
    vector<VROntologyInstance*> res;
    for (auto i : instances) if (i.second->concept->is_a(concept)) res.push_back(i.second);
    return res;
}


