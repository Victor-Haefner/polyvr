#include "VROntology.h"
#include "VRReasoner.h"

#include <iostream>

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

vector<VROntologyRule*> VROntology::getRules() {
    vector<VROntologyRule*> res;
    for (auto r : rules) res.push_back(r.second);
    return res;
}

VROntologyRule* VROntology::addRule(string rule) {
    VROntologyRule* r = new VROntologyRule(rule);
    rules[r->ID] = r;
    return r;
}

VREntity* VROntology::addVectorInstance(string name, string concept, string x, string y, string z) {
    auto i = addInstance(name, concept);
    i->set("x", x);
    i->set("y", y);
    i->set("z", z);
    return i;
}

VREntity* VROntology::addInstance(string name, string concept) {
    auto c = getConcept(concept);
    auto i = new VREntity(name, c);
    instances[i->ID] = i;
    return i;
}

VREntity* VROntology::getInstance(string instance) {
    for (auto i : instances) if (i.second->name == instance) return i.second;
    return 0;
}

vector<VREntity*> VROntology::getInstances(string concept) {
    vector<VREntity*> res;
    for (auto i : instances) if (i.second->concept->is_a(concept)) res.push_back(i.second);
    return res;
}


