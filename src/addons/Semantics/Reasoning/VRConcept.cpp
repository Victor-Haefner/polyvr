#include "VRConcept.h"
#include "VRProperty.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"

#include <iostream>

using namespace OSG;

map<int, VRConceptPtr> VRConcept::ConceptsByID = map<int, VRConceptPtr>();
map<string, VRConceptPtr> VRConcept::ConceptsByName = map<string, VRConceptPtr>();

VRConcept::VRConcept(string name, VROntologyPtr o) {
    //cout << "VRConcept::VRConcept " << name << endl;
    setStorageType("Concept");
    setNameSpace("concept");
    setSeparator('_');
    setUniqueName(false);
    setName(name);
    //this->ontology = o;

    storeMap("Parents", &parents, true);
    storeMap("Properties", &properties, true);
    storeMap("Annotations", &annotations, true);
    regStorageSetupFkt( VRFunction<int>::create("concept setup", boost::bind(&VRConcept::setup, this)) );
}

VRConcept::~VRConcept() {
    //cout << "VRConcept::~VRConcept " << getName() << endl;
}

VRConceptPtr VRConcept::create(string name, VROntologyPtr o) {
    auto c = VRConceptPtr(new VRConcept(name, o));
    ConceptsByID[c->ID] = c;
    ConceptsByName[c->getName()] = c;
    return c;
}

VRConceptPtr VRConcept::ptr() { return shared_from_this(); }

VRConceptPtr VRConcept::copy() {
    auto c = VRConcept::create(name, 0);
    for (auto p : properties) c->addProperty(p.second);
    for (auto a : annotations) c->addAnnotation(a.second);
    //for (auto i : children) c->append(i.second->copy());
    return c;
}

void VRConcept::setup() {
    auto tmp = parents;
    parents.clear();
    for (auto p : tmp) p.second->append(ptr());
}

void VRConcept::removeChild(VRConceptPtr c) {
    //if (children.count(c->ID)) children.erase(c->ID);
    if (c->parents.count(ID)) c->parents.erase(ID);
}

void VRConcept::removeParent(VRConceptPtr c) {
    //if (c->children.count(ID)) c->children.erase(ID);
    if (parents.count(c->ID)) parents.erase(c->ID);
}

void VRConcept::remProperty(VRPropertyPtr p) { if (properties.count(p->ID)) properties.erase(p->ID); }
void VRConcept::addAnnotation(VRPropertyPtr p) { annotations[p->ID] = p; }
void VRConcept::addProperty(VRPropertyPtr p) { properties[p->ID] = p; }

VRConceptPtr VRConcept::append(string name, bool link) {
    auto c = VRConcept::create(name, 0);
    append(c, link);
    return c;
}

void VRConcept::append(VRConceptPtr c, bool link) {
    //cout << "VRConcept::append " << c->getName() << " to " << getName() << " ID " << c->ID << " " << ID << endl;
    //children[c->ID] = c;
    c->parents[ID] = ptr();
    if (!link) return;
    //link[c->ID] = ; // TODO
}

VRPropertyPtr VRConcept::addProperty(string name, string type) {
    auto p = VRProperty::create(name, type);
    addProperty(p);
    return p;
}

VRPropertyPtr VRConcept::addProperty(string name, VRConceptPtr c) { return addProperty(name, c->getName()); }

VRPropertyPtr VRConcept::addAnnotation(string name, string type) {
    auto p = VRProperty::create(name, type);
    addAnnotation(p);
    return p;
}

VRPropertyPtr VRConcept::getProperty(int ID) {
    for (auto p : getProperties()) if (p->ID == ID) return p;
    cout << "Warning: property with ID " << ID << " not found" << endl;
    return 0;
}

VRPropertyPtr VRConcept::getProperty(string name, bool warn) {
    for (auto p : getProperties()) if (p->getName() == name) return p;
    for (auto p : annotations) if (p.second->getName() == name) return p.second;
    if (warn) cout << "Warning: property " << name << " of concept " << this->name << " not found!" << endl;
    return 0;
}

vector<VRPropertyPtr> VRConcept::getProperties(string type) {
    vector<VRPropertyPtr> res;
    for (auto p : getProperties()) {
        if (p->type == type) res.push_back(p);
    }

    if (res.size() == 0) cout << "Warning: no properties of type " << type << " found of concept " << this->name <<  "!" << endl;
    if (res.size() > 1) cout << "Warning: multiple properties of type " << type << " found of concept " << this->name <<  "!" << endl;
    return res;
}

vector<VRPropertyPtr> VRConcept::getProperties() {
    vector<VRPropertyPtr> res;
    map<string, VRPropertyPtr> tmp;
    getProperties(tmp);
    for (auto p : tmp) res.push_back(p.second);
    return res;
}

void VRConcept::getProperties(map<string, VRPropertyPtr>& res) {
    //for (auto p : getParents()) cout << "VRConcept::getParentProperties parent " << p->getName() << " of " << getName() << " Np " << getParents().size() << " ID " << ID << endl;
    for (auto p : getParents()) {
        //cout << "VRConcept::getParentProperties parent " << p->getName() << " of " << getName() << endl;
        p->getProperties(res);
    }
    for (auto p : properties) {
        res[p.second->getName()] = p.second;
    }
}

void VRConcept::detach() {
    for (auto p : getParents()) p->removeChild( ptr() );
    //for (auto c : children) c.second->removeParent( ptr() );
}

bool VRConcept::hasParent(VRConceptPtr c) {
    for (auto p : getParents()) {
        if (p && c == 0) return true;
        if (p && c == p) return true;
    }
    return false;
}

vector<VRConceptPtr> VRConcept::getParents() {
    vector<VRConceptPtr> res;
    for (auto p : parents) res.push_back(p.second);
    return res;
}

int VRConcept::getPropertyID(string name) {
    for (auto p : getProperties()) if (p->getName() == name) return p->ID;
    return -1;
}

bool VRConcept::is_a(VRConceptPtr c) {
    if (ID == c->ID) return true;
    for (auto p : getParents()) if (p->is_a(c)) return true;
    return false;
}

bool VRConcept::is_a(string concept) {
    if (getName() == concept) return true;
    for (auto p : getParents()) if (p->is_a(concept)) return true;
    return false;
}

string VRConcept::toString(string indent) {
    string res = indent+"concept: "+name+"("+::toString(ID)+") - ";
    for (auto p : getParents()) res += p->getName() + " ";
    res += "\n";
    for (auto a : annotations) res += indent+a.second->toString();
    for (auto p : getProperties()) res += indent+p->toString();
    //for (auto c : children) res += c.second->toString(indent+"  ");
    return res;
}

string VRConcept::toString(map<int, vector<VRConceptPtr>>& cMap, string indent) {
    string res = indent+"concept: "+name+"("+::toString(ID)+") - ";
    for (auto p : getParents()) res += p->getName() + " ";
    res += "\n";
    for (auto a : annotations) res += indent+a.second->toString();
    for (auto p : getProperties()) res += indent+p->toString();
    for (auto c : cMap[ID]) res += c->toString(cMap, indent+"  ");
    return res;
}


