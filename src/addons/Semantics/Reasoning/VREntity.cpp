#include "VREntity.h"
#include "VRProperty.h"
#include "VROntology.h"
#include "core/utils/VRStorage_template.h"
#include "core/utils/VRTimer.h"
#include "core/utils/toString.h"

#include <iostream>
#include <OpenSG/OSGVector.h>

using namespace OSG;

VREntity::VREntity() {}

VREntity::VREntity(string name, VROntologyPtr o, VRConceptPtr c) {
    ontology = o;
    //if (!o && c) ontology = c->ontology;
    //if (!o) cout << "Warning: VREntity::VREntity, no valid ontology passed!\n";
    if (c) concepts.push_back(c);

    setStorageType("Entity");
    storeObjNames("concepts", &concepts, &conceptNames);


    auto ns = setNameSpace((o ? "VREntity_"+o->getName() : "VREntity"));
    ns->filterNameChars(".,",'_'); // filter path and math characters
    ns->setSeparator('_');

    // problematic when creating ontology in code, in world gen for example!
    // nice when importing from file, because names are used in properties!
    //ns->setUniqueNames(false);

    setName(name);
    //if (name != getName()) cout << "VREntity::VREntity " << name << " -> " << getName() << endl;
}

VREntityPtr VREntity::create(string name, VROntologyPtr o, VRConceptPtr c) { return VREntityPtr( new VREntity(name, o, c) ); }

void VREntity::setSGObject(VRObjectPtr o) { sgObject = o; }
VRObjectPtr VREntity::getSGObject() { return sgObject.lock(); }

void VREntity::addConcept(VRConceptPtr c) { concepts.push_back(c); }

VRConceptPtr VREntity::getConcept() {
    for (auto cw : concepts) if (auto c = cw.lock()) return c;
    return 0;
}

vector<VRConceptPtr> VREntity::getConcepts() {
    vector<VRConceptPtr> res;
    for (auto cw : concepts) if (auto c = cw.lock()) res.push_back(c);
    return res;
}

vector<string> VREntity::getConceptNames() {
    vector<string> res;
    for (auto cw : concepts) if (auto c = cw.lock()) res.push_back(c->getName());
    return res;
}

string VREntity::getConceptList() {
    string data;
    for (auto n : getConceptNames()) data += n+",";
    if (data.size() > 0) data.pop_back();
    else data += "unknown concept";
    return data;
}

VRPropertyPtr VREntity::getProperty(string name, bool warn) {
    for (auto c : getConcepts()) if (auto p = c->getProperty(name, 0)) return p;
    if (!ontology.lock()) return VRProperty::create(name);
    if (warn) WARN("Warning in VREntity::getProperty: property " + name + " of " + toString() + " not found!");
    return 0;
}

vector<VRPropertyPtr> VREntity::getProperties() {
    vector<VRPropertyPtr> res;
    for (auto c : getConcepts()) for (auto p : c->properties) res.push_back(p.second);
    return res;
}

void VREntity::set(string name, string value, int pos) {
    auto prop = getProperty(name, true);
    if (!prop) { WARN("Warning (set): Entity " + this->name + " has no property " + name); return; }
    if (!properties[name].count(pos)) properties[name][pos] = prop->copy();
    properties[name][pos]->setValue( value );
}

void VREntity::add(string name, string value) {
    auto prop = getProperty(name, true);
    if (!prop) { WARN("Warning (add): Entity " + this->name + " has no property " + name); return; }
    prop = prop->copy();
    prop->setValue( value );
    int i = properties[name].size();
    properties[name][i] = prop;
}

void VREntity::clear(string name) {
    auto prop = getProperty(name, true);
    if (!prop) { WARN("Warning (clear): Entity " + this->name + " has no property " + name); return; }
    properties[name].clear();
}

void VREntity::rem(VRPropertyPtr p) {
    string name = p->getName();
    if (properties.count(name)) {
        auto& m = properties[name];
        vector<int> keys;
        for (auto k : m) if (k.second == p) keys.push_back(k.first);
        for (auto k : keys) m.erase(k);
    }
}

void VREntity::setVector(string name, vector<string> v, string type, int pos) {
    if (auto o = ontology.lock()) {
        if (!properties.count(name)) { addVector(name, v, type); return; }
        if (!get(name)) { WARN("Warning (setVector): Entity " + this->name + " has no property " + name); return; }
        auto v_name = properties[name][pos]->value;
        auto vec = o->getEntity(v_name);
        if (!vec) { WARN("Warning (setVector): Entity " + this->name + " has no vector entity " + v_name); return; }
        int N = v.size();
        if (0 < N) vec->set("x", v[0]);
        if (1 < N) vec->set("y", v[1]);
        if (2 < N) vec->set("z", v[2]);
        if (3 < N) vec->set("w", v[3]);
    }
}

void VREntity::addVector(string name, vector<string> v, string type) {
    if (auto o = ontology.lock()) {
        string v_name = this->name+"_"+name;
        auto e = o->addVectorEntity(v_name, type, v);
        add(name, e->getName());
    }
}

void VREntity::setVec3(string name, Vec3d v, string type, int pos) {
    setVector(name, { ::toString(v[0]), ::toString(v[1]), ::toString(v[2]) }, type, pos);
}

void VREntity::addVec3(string name, Vec3d v, string type) {
    addVector(name, { ::toString(v[0]), ::toString(v[1]), ::toString(v[2]) }, type);
}


vector<VRPropertyPtr> VREntity::getVector(const string& prop, int i) { // TODO
    vector<VRPropertyPtr> res;
    auto vp = get(prop, i);
    if (!vp) return res;
    if (auto o = ontology.lock()) {
        auto ve = o->getEntity( vp->value ); // vector entity
        if (auto p = ve->get("x")) res.push_back( p );
        if (auto p = ve->get("y")) res.push_back( p );
        if (auto p = ve->get("z")) res.push_back( p );
        if (auto p = ve->get("w")) res.push_back( p );
    }
    return res;
}

vector< vector<VRPropertyPtr> > VREntity::getAllVector(const string& prop) { // TODO
    vector< vector<VRPropertyPtr> > res;
    return res;
}

vector<VRPropertyPtr> VREntity::getAll(string name) {
    vector<VRPropertyPtr> res;
    if (name != "" && properties.count(name)) {
        for (auto k : properties[name]) res.push_back(k.second);
    }
    if (name == "") for (auto pv : properties) for (auto p : pv.second) res.push_back(p.second);
    return res;
}

VRPropertyPtr VREntity::get(const string& prop, int i) {
    auto props = getAll(prop);
    if (i < 0) i += props.size();
    if (i >= int(props.size())) return 0;
    return props[i];
}

VRPropertyValue VREntity::getStringValue(string prop, int i) {
    return VRPropertyValue(get(prop, i), ontology.lock());
}

vector<VRPropertyValue> VREntity::getAllStringValues(string prop) {
    vector<VRPropertyValue> res;
    for (auto p : getAll(prop)) res.push_back( VRPropertyValue(p, ontology.lock()) );
    return res;
}

vector<VRPropertyValue> VREntity::getStringVector(string prop, int i) {
    vector<VRPropertyValue> res;
    for (auto p : getVector(prop, i)) res.push_back( VRPropertyValue(p, ontology.lock()) );
    return res;
}

vector< vector<VRPropertyValue> > VREntity::getAllStringVector(string prop) {
    vector< vector<VRPropertyValue> > res;
    for (auto pv : getAllVector(prop)) {
        vector<VRPropertyValue> r;
        for (auto p : pv) r.push_back( VRPropertyValue(p, ontology.lock()) );
        res.push_back(r);
    }
    return res;
}

VREntityPtr VREntity::getEntity(const string& prop, int i) {
    auto p = get(prop, i);
    if (!p) return 0;
    return ontology.lock()->getEntity( p->value );
}

vector<VREntityPtr> VREntity::getAllEntities(const string& prop) {
    vector<VREntityPtr> res;
    for (auto p : getAll(prop)) {
        auto e = ontology.lock()->getEntity( p->value );
        if (e) res.push_back( e );
    }
    return res;
}

Vec3d VREntity::getVec3(const string& prop, int i) {
    Vec3d res;
    auto vec = getVector(prop, i);
    int N = vec.size(); N = min(N,3);
    for (int i=0; i<N; i++) res[i] = toFloat( vec[i]->value );
    return res;
}

vector< Vec3d > VREntity::getAllVec3(const string& prop) { // TODO
    vector< Vec3d > res;
    return res;
}

string VREntity::asVectorString() {
    return "[" + get("x")->value +","+ get("y")->value +","+ get("z")->value+"]";
}

VREntityPtr VREntity::copy() {
    auto o = ontology.lock();
    auto e = create(getBaseName(), o);
    o->addEntity(e);
    for (auto wc : concepts) if (auto c = wc.lock()) e->addConcept(c);
    e->conceptNames = conceptNames;
    for (auto pv : properties) {
        e->properties[pv.first] = map<int, VRPropertyPtr>();
        for (auto k : pv.second) e->properties[pv.first][k.first] = k.second->copy();
    }
    map<string, vector<VRPropertyPtr> > properties;
    return e;
}

string VREntity::toString() {
    string data = "Entity " + name + " of type (";
    data += getConceptList() + ")";
    data += " with properties:";
    for (auto p : properties) {
        for (auto sp : p.second) {
            data += " "+sp.second->getName()+"("+sp.second->type+")="+sp.second->value;
        }
    }
    return data;
}

bool VREntity::is_a(string concept) {
    for (auto cw : concepts) {
        if (auto c = cw.lock()) {
            if (c->is_a(concept)) return true;
        }
    }
    return false;
}

void VREntity::save(XMLElementPtr e, int p) {
    VRStorage::save(e,p);
    e = e->addChild("properties");
    for (auto p : properties) {
        auto e2 = e->addChild(p.first);
        for (auto sp : p.second) {
            auto e3 = e2->addChild(sp.second->getName());
            e3->setAttribute("value", sp.second->value);
            e3->setAttribute("type", sp.second->type);
        }
    }
}

void VREntity::load(XMLElementPtr e) {
    VRStorage::load(e);
    e = e->getChild("properties");
    for (auto el : e->getChildren()) {
        for (auto el2 : el->getChildren()) {
            string n = el2->getName();
            auto p = VRProperty::create(n,"");
            if (el2->hasAttribute("value")) p->setValue( el2->getAttribute("value") );
            if (el2->hasAttribute("type")) p->setType( el2->getAttribute("type") );
            int i = properties[n].size();
            properties[n][i] = p;
        }
    }
}
