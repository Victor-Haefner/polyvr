#include "VROntology.h"
#include "VRReasoner.h"
#include "VRProperty.h"
#include "VROWLImport.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRSemanticManager.h"
#include <iostream>

using namespace OSG;

VROntology::VROntology(string name) {
    setStorageType("Ontology");
    setPersistency(0);
    setNameSpace("Ontology");
    setName(name);

    storeMap("Instances", &instances, true);
    storeMap("Rules", &rules, true);
    store("flag", &flag);
    regStorageSetupFkt( VRFunction<int>::create("ontology setup", boost::bind(&VROntology::setup, this)) );
}

VROntologyPtr VROntology::create(string name) {
    auto o = VROntologyPtr( new VROntology(name) );
    o->thing = VRConcept::create("Thing", o);
    o->concepts["Thing"] = o->thing;
    o->storeObj("Thing", o->thing);
    return o;
}

VROntologyPtr VROntology::ptr() { return shared_from_this(); }

void VROntology::setup() {
    vector<VRConceptPtr> cpts;
    thing->getDescendance(cpts);
    for (auto& c : cpts) concepts[c->getName()] = c;

    auto insts = instances;
    instances.clear();
    for (auto& i : insts) {
        for (auto c : i.second->conceptNames) i.second->addConcept( concepts[c].lock() ); // update concept
        instances[i.second->ID] = i.second; // update ID mapping
    }

    auto rls = rules;
    rules.clear();
    for (auto& r : rls) {
        rules[r.second->ID] = r.second; // update ID mapping
    }
}

VRConceptPtr VROntology::getConcept(string name) {
    /*cout << "add concept " << name << "in: ";
    for (auto c : concepts) if (auto p = c.second.lock()) cout << " " << c.first << " " << p->ID << " ";
    cout << endl;*/
    VRConceptPtr p;
    if (concepts.count(name)) p = concepts[name].lock();
    else {
        for (auto ow : dependencies) {
            if (auto o = ow.second.lock()) {
                p = o->getConcept(name);
                if (p) {
                    p = p->copy();
                    addConcept(p);
                    break;
                }
            }
        }
    }
    //cout << "found " << p->name << " " << p->ID << endl;
    //if (!p) cout << "Warning: concept " << name << " not found in ontology " << getName() << endl;
    return p;
}

vector<VRConceptPtr> VROntology::getConcepts() {
    vector<VRConceptPtr> res;
    for (auto c : concepts) res.push_back(c.second.lock());
    return res;
}

VRConceptPtr VROntology::addConcept(string concept, string parent) {
    if (concepts.count(concept)) { cout << "WARNING in VROntology::addConcept, " << concept << " known, skipping!\n"; return 0;  }

    auto p = thing;
    if (parent != "") {
        p = getConcept(parent);
        if (!p) { cout << "WARNING in VROntology::addConcept, " << parent << " not found while adding " << concept << "!\n"; return 0;  }
    }
    //cout << "VROntology::addConcept " << concept << " " << parent << " " << p->name << " " << p->ID << endl;
    p = p->append(concept);
    addConcept(p);
    return p;
}

void VROntology::addConcept(VRConceptPtr c) {
    if (concepts.count(c->getName())) { cout << "WARNING in VROntology::addConcept, " << c->getName() << " known, skipping!\n"; return;  }
    if (c == thing) return;
    concepts[c->getName()] = c;
    if (!c->parent.lock()) thing->append(c);
}

void VROntology::remConcept(VRConceptPtr c) {
    if (c == thing) return;
    if (!concepts.count(c->getName())) return;
    if (auto p = c->parent.lock()) p->remove(c);
    concepts.erase(c->getName());
}

void VROntology::renameConcept(VRConceptPtr c, string newName) {
    if (c == thing) return;
    if (!concepts.count(c->getName())) return;
    concepts.erase(c->getName());
    c->setName(newName);
    addConcept(c);
}

void VROntology::remEntity(VREntityPtr e) {
    for (auto i : instances) cout << "i " << i.first << " " << i.second->getName() << " " << i.second->ID << endl;
    if (!instances.count(e->ID)) return;
    cout << "VROntology::remEntity " << e->getName() << " " << e->ID << endl;
    instances.erase(e->ID);
}

void VROntology::remRule(VROntologyRulePtr r) {
    if (!rules.count(r->ID)) return;
    rules.erase(r->ID);
}

void VROntology::renameEntity(VREntityPtr e, string s) {
    if (!instances.count(e->ID)) return;
    e->setName(s);
}

void VROntology::import(VROntologyPtr o) { dependencies[o->getName()] = o; }

void VROntology::merge(VROntologyPtr o) { // Todo: check it well!
    for (auto c : o->rules) rules[c.first] = c.second;
    for (auto c : o->thing->children) {
        auto cn = c.second->copy();
        thing->append(cn);
        vector<VRConceptPtr> cpts;
        cn->getDescendance(cpts);
        for (auto c : cpts) {
            concepts[c->getName()] = c;
            c->ontology = shared_from_this();
        }
    }
}

VROntologyPtr VROntology::copy() {
    auto o = create(name);
    o->merge(shared_from_this());
    return o;
}

vector<VROntologyRulePtr> VROntology::getRules() {
    vector<VROntologyRulePtr> res;
    for (auto r : rules) res.push_back(r.second);
    return res;
}

VROntologyRulePtr VROntology::addRule(string rule, string ac) {
    VROntologyRulePtr r = VROntologyRule::create(rule, ac);
    rules[r->ID] = r;
    return r;
}

VREntityPtr VROntology::addVectorInstance(string name, string concept, string x, string y, string z) {
    vector<string> v;
    v.push_back(x); v.push_back(y); v.push_back(z);
    return addVectorInstance(name, concept, v);
}

VREntityPtr VROntology::addVectorInstance(string name, string concept, vector<string> val) {
    auto i = addInstance(name, concept);
    int N = val.size();
    if (0 < N) i->set("x", val[0]);
    if (1 < N) i->set("y", val[1]);
    if (2 < N) i->set("z", val[2]);
    if (3 < N) i->set("w", val[3]);
    cout << "addVectorInstance " << name << " " << concept << endl;
    return i;
}

void VROntology::addInstance(VREntityPtr e) { instances[e->ID] = e; }

VREntityPtr VROntology::addInstance(string name, string concept) {
    auto c = getConcept(concept);
    auto e = VREntity::create(name, ptr(), c);
    addInstance(e);
    return e;
}

VREntityPtr VROntology::getInstance(string instance) {
    for (auto i : instances) if (i.second->getName() == instance) return i.second;
    return 0;
}

vector<VREntityPtr> VROntology::getInstances(string concept) {
    vector<VREntityPtr> res;
    if (concept != "") {
        for (auto i : instances) {
            for (auto c : i.second->getConcepts()) {
                if(c && c->is_a(concept)) { res.push_back(i.second); break; }
            }
        }
    } else for (auto i : instances) res.push_back(i.second);
    return res;
}

string VROntology::toString() {
    string res = "Taxonomy:\n";
    res += thing->toString();
    res += "Entities:\n";
    for (auto e : instances) res += e.second->toString() + "\n";
    return res;
}

void VROntology::open(string path) {
    VROWLImport importer;
    importer.load(shared_from_this(), path);
}

void VROntology::addModule(string mod) {
    auto mgr = VRSceneManager::getCurrent()->getSemanticManager();
    auto onto = mgr->getOntology(mod);
    if (!onto) { cout << "VROntology::addModule Error: Ontology " << mod << " not found" << endl; return; }
    merge(onto);
}

void VROntology::setFlag(string f) { flag = f; }
string VROntology::getFlag() { return flag; }

