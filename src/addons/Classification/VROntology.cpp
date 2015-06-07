#include "VROntology.h"
#include "VRReasoner.h"

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

VROntologyRule::VROntologyRule(string rule) {
    this->name = "rule";
    this->rule = rule;
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

VROntology::VROntology() {
    thing = new VRConcept("Thing");
}

VRConcept* VROntology::getConcept(string name, VRConcept* p) {
    if (p == 0) p = thing;
    if (p->name == name) return p;
    VRConcept* c = 0;
    for (auto ci : p->children) {
        c = getConcept(name, ci.second);
        if (c) return c;
    }
    return c;
}

VRConcept* VROntology::addConcept(string concept, string parent) {
    if (parent == "") return thing->append(concept);
    auto p = getConcept(parent);
    if (p == 0) { cout << "WARNING in VROntology::addConcept, " << parent << " not found while adding " << concept << "!\n"; return 0;  }
    return getConcept(parent)->append(concept);
}

string VROntology::answer(string question) {
    auto res = VRReasoner::get()->process(question, this);
    return "";//res.toString();
}

void VROntology::merge(VROntology* o) {
    for (auto c : o->thing->children)
        thing->append(c.second);
    for (auto c : o->rules)
        rules[c.first] = c.second;
}

int VRConcept::getPropertyID(string name) {
    for (auto p : properties) if (p.second->name == name) return p.second->ID;
    return -1;
}

vector<VROntologyRule*> VROntology::getRules() {
    vector<VROntologyRule*> res;
    for (auto r : rules) res.push_back(r.second);
    return res;
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

vector<string> VROntologyInstance::getAtPath(vector<string> path) {
    cout << "  get value at path ";
    for (auto p : path) cout << "/" << p;
    cout << endl;

    vector<string> res;

    if (path.size() == 2) {
        string m = path[1];
        int id = concept->getPropertyID(m);
        cout << "  get value of member " << m << " with id " << id << endl;
        if (id < 0) return res;
        if (!properties.count(id)) return res;
        return properties[id];
    }

    return res;
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

VROntologyRule* VROntology::addRule(string rule) {
    VROntologyRule* r = new VROntologyRule(rule);
    rules[r->ID] = r;
    return r;
}

VROntologyInstance* VROntology::addVectorInstance(string name, string concept, string x, string y, string z) {
    auto i = addInstance(name, concept);
    i->set("x", x);
    i->set("y", y);
    i->set("z", z);
    return i;
}

bool VRConcept::is_a(string concept) {
    VRConcept* c = this;
    while (c) {
        if (c->name == concept) return true;
        c = c->parent;
    }
    return false;
}

VROntologyInstance* VROntology::addInstance(string name, string concept) {
    auto c = getConcept(concept);
    auto i = new VROntologyInstance(name, c);
    instances[i->ID] = i;
    return i;
}

VROntologyInstance* VROntology::getInstance(string instance) {
    for (auto i : instances) if (i.second->name == instance) return i.second;
    return 0;
}

vector<VROntologyInstance*> VROntology::getInstances(string concept) {
    vector<VROntologyInstance*> res;
    for (auto i : instances) if (i.second->concept->is_a(concept)) res.push_back(i.second);
    return res;
}


