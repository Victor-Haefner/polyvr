#include "VREntity.h"
#include "VRProperty.h"
#include "VROntology.h"
#include "core/utils/VRStorage_template.h"
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"

#include <iostream>

using namespace OSG;

#define WARN(x) \
VRGuiManager::get()->getConsole( "Errors" )->write( x+"\n" );

/*#include <libxml++/nodes/element.h>
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include <boost/bind.hpp>
template<typename T>
void VRStorage::save_vec_on_cb(vector<T>* v, string tag, xmlpp::Element* e) {
    e = e->add_child(tag);
    for (auto t : *v) {
        auto sp = t.lock();
        if (sp == 0) continue;
        auto e2 = e->add_child("e");
        e2->set_attribute("val", sp->getName());
    }
}

template<typename T>
void VRStorage::load_vec_cb(vector<T>* v, string tag, xmlpp::Element* e) {
    if (e == 0) return;
    e = getChild(e, tag);
    if (e == 0) return;
    for (auto el : getChildren(e)) {
        if (el->get_attribute("val") == 0) return;
        T t;
        toValue( el->get_attribute("val")->get_value(), t);
        v->push_back( t );
    }
}

template<typename T>
void VRStorage::storeObjNames(string tag, vector<T>* o, vector<string>* v) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", boost::bind( &VRStorage::load_vec_cb<string>, this, v, tag, _1 ) );
    b.f2 = VRStoreCb::create("save", boost::bind( &VRStorage::save_vec_on_cb<T>, this, o, tag, _1 ) );
    storage[tag] = b;
}*/

VREntity::VREntity(string name, VROntologyPtr o, VRConceptPtr c) {
    ontology = o;
    //if (!o && c) ontology = c->ontology;
    if (!o) cout << "Warning: VREntity::VREntity, no valid ontology passed!\n";
    concepts.push_back(c);

    setStorageType("Entity");
    storeObjNames("concepts", &concepts, &conceptNames);
    filterNameChars(".,",'_');

    setNameSpace("entity");
    setSeparator('_');
    setName(name);
}

VREntityPtr VREntity::create(string name, VROntologyPtr o, VRConceptPtr c) { return VREntityPtr( new VREntity(name, o, c) ); }

void VREntity::setSGObject(VRObjectPtr o) { sgObject = o; }
VRObjectPtr VREntity::getSGObject() { return sgObject.lock(); }

void VREntity::addConcept(VRConceptPtr c) { concepts.push_back(c); }

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

VRPropertyPtr VREntity::getProperty(string name) {
    for (auto c : getConcepts()) if (auto p = c->getProperty(name, 0)) return p;
    WARN("Warning: property " + name + " of entity " + this->name + " not found!");
    return 0;
}

vector<VRPropertyPtr> VREntity::getProperties() {
    vector<VRPropertyPtr> res;
    for (auto c : getConcepts()) for (auto p : c->properties) res.push_back(p.second);
    return res;
}

void VREntity::set(string name, string value, int pos) {
    if (!properties.count(name)) { add(name, value); return; }
    auto prop = get(name);
    if (!prop) { WARN("Warning (set): Entity " + this->name + " has no property " + name); return; }
    properties[name][pos]->value = value;
    // TODO: warn if vector size bigger 1
}

void VREntity::add(string name, string value) {
    auto prop = getProperty(name);
    if (!prop) { WARN("Warning (add): Entity " + this->name + " has no property " + name); return; }
    prop = prop->copy();
    prop->value = value;
    properties[name].push_back( prop );
}

void VREntity::clear(string name) {
    auto prop = getProperty(name);
    if (!prop) { WARN("Warning (clear): Entity " + this->name + " has no property " + name); return; }
    properties[name].clear();
}

void VREntity::rem(VRPropertyPtr p) {
    string name = p->getName();
    if (properties.count(name)) {
        auto& v = properties[name];
        v.erase( remove(v.begin(), v.end(), p), v.end() );
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

VRPropertyPtr VREntity::get(string prop, int i) {
    auto props = getAll(prop);
    if (i >= int(props.size())) return 0;
    return props[i];
}

vector<VRPropertyPtr> VREntity::getAll(string name) {
    if (name != "" && properties.count(name)) return properties[name];
    vector<VRPropertyPtr> res;
    if (name == "") for (auto pv : properties) for (auto p : pv.second) res.push_back(p);
    return res;
}

vector<VRPropertyPtr> VREntity::getVector(string prop, int i) { // TODO
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

vector< vector<VRPropertyPtr> > VREntity::getAllVector(string prop) { // TODO
    vector< vector<VRPropertyPtr> > res;
    return res;
}

VREntityPtr VREntity::getEntity(string prop, int i) {
    auto p = get(prop, i);
    if (!p) return 0;
    if (auto onto = ontology.lock()) return onto->getEntity( p->value );
    return 0;
}

vector<VREntityPtr> VREntity::getAllEntities(string prop) {
    vector<VREntityPtr> res;
    for (auto p : getAll(prop)) {
        auto e = ontology.lock()->getEntity( p->value );
        if (e) res.push_back( e );
    }
    return res;
}

Vec3f VREntity::getVec3f(string prop, int i) {
    Vec3f res;
    auto vec = getVector(prop, i);
    for (int i=0; i<3; i++) res[i] = toFloat( vec[i]->value );
    return res;
}

vector< Vec3f > VREntity::getAllVec3f(string prop) { // TODO
    vector< Vec3f > res;
    return res;
}


string VREntity::toString() {
    string data = "Entity " + name + " of type (";
    data += getConceptList() + ")";
    data += " with properties:";
    for (auto p : properties) {
        for (auto sp : p.second) {
            data += " "+sp->getName()+"("+sp->type+")="+sp->value;
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

void VREntity::save(xmlpp::Element* e, int p) {
    VRStorage::save(e,p);
    e = e->add_child("properties");
    for (auto p : properties) {
        auto e2 = e->add_child(p.first);
        for (auto sp : p.second) {
            auto e3 = e2->add_child(sp->getName());
            e3->set_attribute("value", sp->value);
            e3->set_attribute("type", sp->type);
        }
    }
}

void VREntity::load(xmlpp::Element* e) {
    VRStorage::load(e);
    e = getChild(e, "properties");
    for (auto el : getChildren(e)) {
        for (auto el2 : getChildren(el)) {
            string n = el2->get_name();
            auto p = VRProperty::create(n,"");
            if (el2->get_attribute("value")) p->value = el2->get_attribute("value")->get_value();
            if (el2->get_attribute("type")) p->type = el2->get_attribute("type")->get_value();
            properties[n].push_back(p);
        }
    }
}
