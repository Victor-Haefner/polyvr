#include "VRSemanticManager.h"
#include "addons/Semantics/Reasoning/VROntology.h"
//#include "addons/Semantics/Reasoning/VROntologyLibrary.cpp"

#include <iostream>

using namespace OSG;
using namespace std;

VRSemanticManager::VRSemanticManager() {
    cout << "Init VRSemanticManager\n";

    for (auto o : VROntology::library) ontologies[o.first] = o.second;
}

VRSemanticManager::~VRSemanticManager() {}

VRSemanticManagerPtr VRSemanticManager::create() { return VRSemanticManagerPtr(new VRSemanticManager()); }

VROntologyPtr VRSemanticManager::addOntology(string name) { ontologies[name] = VROntology::create(name); return ontologies[name];}
VROntologyPtr VRSemanticManager::loadOntology(string path) { auto o = addOntology(path); o->open(path); return o; }
VROntologyPtr VRSemanticManager::getOntology(string name) { return ontologies.count(name) ? ontologies[name] : 0; }

vector<VROntologyPtr> VRSemanticManager::getOntologies() {
    vector<VROntologyPtr> res;
    for (auto o : ontologies) res.push_back(o.second);
    return res;
}

void VRSemanticManager::renameOntology(string name, string new_name) {
    auto o = ontologies[name];
    o->setName(new_name);
    ontologies.erase(name);
    ontologies[new_name] = o;
}
