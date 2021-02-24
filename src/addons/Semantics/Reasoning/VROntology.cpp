#include "VROntology.h"
#include "VRReasoner.h"
#include "VRProperty.h"
#ifndef WITHOUT_RAPTOR
#include "VROWLImport.h"
#endif
#include "VROWLExport.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSemanticManager.h"
#ifndef WITHOUT_GTK
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"
#endif
#include <iostream>

#ifndef WITHOUT_GTK
#define WARN(x) \
VRGuiManager::get()->getConsole( "Errors" )->write( x+"\n" );
#else
#define WARN(x) \
std::cout << x << std::endl;
#endif

using namespace OSG;

VROntology::VROntology(string name) {
    setStorageType("Ontology");
    setPersistency(0);
    setNameSpace("Ontology");
    setName(name);

    storeMap("Entities", &entities, true);
    storeMap("Rules", &rules, true);
    store("flag", &flag);
    regStorageSetupFkt( VRStorageCb::create("ontology setup", bind(&VROntology::setup, this, placeholders::_1)) );
}

VROntologyPtr VROntology::create(string name) {
    auto o = VROntologyPtr( new VROntology(name) );
    static VRConceptPtr thing = VRConcept::create("Thing", o);
    o->thing = thing;
    o->concepts["Thing"] = o->thing;
    o->storeObj("Thing", o->thing);
    auto f = o->addConcept("float");
    auto i = o->addConcept("int");
    auto s = o->addConcept("string");
    f->addProperty("var", "float");
    i->addProperty("var", "int");
    s->addProperty("var", "string");
    return o;
}

VROntologyPtr VROntology::ptr() { return shared_from_this(); }

void VROntology::setup(VRStorageContextPtr context) {
    //map<int, VRConceptPtr> cpts;
    //thing->getDescendance(cpts);
    //for (auto& c : cpts) concepts[c.second->getName()] = c.second;

    auto insts = entities;
    entities.clear();
    for (auto& i : insts) {
        for (auto c : i.second->conceptNames) i.second->addConcept( concepts[c].lock() ); // update concept
        entities[i.second->ID] = i.second; // update ID mapping
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
    if (recentConcepts.count(name)) return recentConcepts[name];

    VRConceptPtr p;
    if (concepts.count(name)) p = concepts[name].lock();
    else {
        for (auto ow : dependencies) {
            if (auto o = ow.second.lock()) {
                p = o->getConcept(name);
                if (p) {
                    //p = p->copy();
                    addConcept(p);
                    break;
                }
            }
        }
    }
    //cout << "found " << p->name << " " << p->ID << endl;
    //if (!p) cout << "Warning: concept " << name << " not found in ontology " << getName() << endl;
    if (p) {
        if (recentConcepts.size() > 10) { // too big, remove random element
            auto it = recentConcepts.begin();
            advance(it, rand() % recentConcepts.size());
            recentConcepts.erase(it);
        }
        recentConcepts[name] = p;
    }
    return p;
}

VRPropertyPtr VROntology::getProperty(string prop) {
    if (properties.count(prop)) return properties[prop];
    return 0;
}

vector<VRConceptPtr> VROntology::getConcepts() {
    vector<VRConceptPtr> res;
    for (auto c : concepts) res.push_back(c.second.lock());
    return res;
}

VRConceptPtr VROntology::addConcept(string concept, string parents, map<string, string> props, string comment) {
    if (concepts.count(concept) && concept != "Thing") { cout << "WARNING in VROntology::addConcept, " << concept << " known, skipping!\n"; return 0;  }

    vector<VRConceptPtr> Parents;
    if (parents != "") {
        for (auto parent : splitString(parents, ' ')) {
            auto p = getConcept(parent);
            if (!p) { WARN("WARNING in VROntology::addConcept, " + parent + " not found while adding " + concept); return 0;  }
            Parents.push_back(p);
        }
    }

    VRConceptPtr Concept;
    if (Parents.size() == 0) Concept = thing->append(concept);
    else Concept = Parents[0]->append(concept);

    //cout << "VROntology::addConcept " << concept << " " << parents << " " << Concept->getName() << " " << Concept->ID << endl;
    for (unsigned int i=1; i<Parents.size(); i++) Parents[i]->append(Concept);
    Concept->addAnnotation(comment, "comment");
    addConcept(Concept);
    for (auto p : props) Concept->addProperty(p.first, p.second);
    return Concept;
}

void VROntology::addConcept(VRConceptPtr c) {
    if (concepts.count(c->getName()) && c->getName() != "Thing") { WARN("WARNING in VROntology::addConcept, " + c->getName() + " known, skipping!"); return;  }
    if (c == thing) return;
    concepts[c->getName()] = c;
    if (!c->hasParent()) thing->append(c);
}

void VROntology::addProperty(VRPropertyPtr p) {
    if (properties.count(p->getName())) { WARN("WARNING in VROntology::addProperty, " + p->getName() + " known, skipping!"); return;  }
    properties[p->getName()] = p;
}

void VROntology::remConcept(VRConceptPtr c) {
    if (c == thing) return;
    if (!concepts.count(c->getName())) return;
    c->detach();
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
    if (!e) return;
    if (!entities.count(e->ID)) return;
    entities.erase(e->ID);
}

void VROntology::remEntity(string name) {
    remEntity(getEntity(name));
}

void VROntology::remEntities(string concept) {
    for (auto e : getEntities(concept)) remEntity(e);
}

void VROntology::remRule(VROntologyRulePtr r) {
    if (!rules.count(r->ID)) return;
    rules.erase(r->ID);
}

void VROntology::renameEntity(VREntityPtr e, string s) {
    if (!entities.count(e->ID)) return;
    e->setName(s);
}

void VROntology::merge(VROntologyPtr o) { // Todo: check it well!
    for (auto c : o->rules) rules[c.first] = c.second;
    for (auto b : o->builtins) builtins[b.first] = b.second;
    for (auto c : o->concepts) {
        auto cn = c.second.lock();
        if (cn) concepts[cn->getName()] = cn;
    }
}

map<int, vector<VRConceptPtr>> VROntology::getChildrenMap() {
    map<int, vector<VRConceptPtr>> res;
    for (auto wc : concepts) {
        auto c = wc.second.lock();
        if (c) for (auto p : c->parents) res[p.second->ID].push_back(c);
    }
    return res;
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
    for (auto r : rules) if (r.second->rule == rule) return r.second;
    VROntologyRulePtr r = VROntologyRule::create(rule, ac);
    rules[r->ID] = r;
    return r;
}

VREntityPtr VROntology::addVectorEntity(string name, string concept, string x, string y, string z) {
    vector<string> v;
    v.push_back(x); v.push_back(y); v.push_back(z);
    return addVectorEntity(name, concept, v);
}

VREntityPtr VROntology::addVectorEntity(string name, string concept, vector<string> val) {
    auto i = addEntity(name, concept);
    int N = val.size();
    if (0 < N) i->set("x", val[0]);
    if (1 < N) i->set("y", val[1]);
    if (2 < N) i->set("z", val[2]);
    if (3 < N) i->set("w", val[3]);
    return i;
}

void VROntology::addEntity(VREntityPtr& e) {
    if (!e) return;
    //pair<int, VREntityPtr> p1(e->ID,0);
    //auto p2 = entities.insert(p1);
    //p2.first->second = e;
    entities[e->ID] = e;
    entitiesByName[e->getName()] = e;

    //cout << "VROntology::addEntity " << entities.size() << " " << entities[e->ID] << endl;
}

VREntityPtr VROntology::addEntity(string name, string concept, map<string, string> props) {
    auto c = getConcept(concept);
    auto e = VREntity::create(name, ptr(), c);
    addEntity(e);
    for (auto p : props) e->set(p.first, p.second);
    /*int ID = guid();
    auto p = entities.emplace(ID, name, ptr(), c);
    VREntityPtr e = p.first->second;
    e->ID = ID;*/
    return e;
}

VREntityPtr VROntology::getEntity(int ID) {
    if (!entities.count(ID)) return 0;
    return entities[ID];
}

VREntityPtr VROntology::getEntity(string e) {
    if (!entitiesByName.count(e)) return 0;
    return entitiesByName[e];
}

vector<VREntityPtr> VROntology::getEntities(string concept) {
    vector<VREntityPtr> res;
    if (concept != "") {
        for (auto i : entities) if (i.second->is_a(concept)) res.push_back(i.second);
    } else for (auto i : entities) res.push_back(i.second);
    return res;
}

string VROntology::toString() {
    string res = "Taxonomy:\n";
    auto cMap = getChildrenMap();
    res += thing->toString(cMap);
    res += "Entities:\n";
    for (auto e : entities) res += e.second->toString() + "\n";
    res += "Rules:\n";
    for (auto r : rules) res += r.second->toString() + "\n";
    return res;
}

void VROntology::openOWL(string path) {
#ifndef WITHOUT_RAPTOR
    if (!exists(path)) WARN("WARNING in VROntology::openOWL, " + path + " not found!");
    VROWLImport importer;
    importer.read(shared_from_this(), path);
#endif
}

void VROntology::saveOWL(string path) {
    VROWLExport exporter;
    exporter.write(shared_from_this(), path);
}

void VROntology::addModule(VROntologyPtr onto) {
    string mod = onto->getBaseName();
    if (!onto) { cout << "VROntology::addModule Error: Ontology " << mod << " not found" << endl; return; }
    merge(onto);
    modules[mod] = onto;

    function<void (VROntologyPtr)> regMods = [&](VROntologyPtr o) {
        if (!o) return;
        for (auto s : o->getModules()) {
            modules[s.first] = s.second;
            regMods(s.second);
        }
    };
    regMods(onto);
}

void VROntology::addModule(string mod) {
    auto mgr = VRScene::getCurrent()->getSemanticManager();
    addModule( mgr->getOntology(mod) );
}

void VROntology::loadModule(string path) {
    if (!exists(path)) WARN("WARNING in VROntology::openOWL, " + path + " not found!");
    auto mgr = VRScene::getCurrent()->getSemanticManager();
    addModule( mgr->loadOntology(path) );
}

map<string, VROntologyPtr> VROntology::getModules() {
    map<string, VROntologyPtr> res;
    for (auto o : modules) if (auto O = o.second.lock()) res[o.first] = O;
    return res;
}

void VROntology::setFlag(string f) { flag = f; }
string VROntology::getFlag() { return flag; }

vector<VREntityPtr> VROntology::process(string query, bool allowAssumptions) {
    auto r = VRReasoner::create();
    r->setOption("allowAssumptions", allowAssumptions);
    return r->process(query, ptr());
}

VREntityPtr VROntology::addVec3Entity(string name, string concept, Vec3d v) {
    return addVectorEntity(name, concept, {::toString(v[0]), ::toString(v[1]), ::toString(v[2])});
}


