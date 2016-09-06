#include "VREntity.h"
#include "VRProperty.h"
#include "VROntology.h"
#include "core/utils/VRStorage_template.h"

#include <iostream>

using namespace OSG;

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
    if (!o && c) ontology = c->ontology;
    concepts.push_back(c);

    setStorageType("Entity");
    storeObjNames("concepts", &concepts, &conceptNames);

    setNameSpace("entity");
    setName(name);
}

VREntityPtr VREntity::create(string name, VROntologyPtr o, VRConceptPtr c) { return VREntityPtr( new VREntity(name, o, c) ); }

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
    if (concepts.size() > 0) data.pop_back();
    else data += "unknown concept";
    return data;
}

VRPropertyPtr VREntity::getProperty(string name) {
    for (auto c : getConcepts()) if (auto p = c->getProperty(name, 0)) return p;
    cout << "Warning: property " << name << " of entity " << this->name << " not found!" << endl;
    return 0;
}

vector<VRPropertyPtr> VREntity::getProperties() {
    vector<VRPropertyPtr> res;
    for (auto c : getConcepts()) for (auto p : c->properties) res.push_back(p.second);
    return res;
}

void VREntity::set(string name, string value) {
    if (!properties.count(name)) { add(name, value); return; }
    auto prop = getProperty(name);
    if (!prop) { cout << "Warning (set): Entity " << this->name << " has no property " << name << endl; return; }
    properties[name][0]->value = value;
    // TODO: warn if vector size bigger 1
}

void VREntity::add(string name, string value) {
    auto prop = getProperty(name);
    if (!prop) { cout << "Warning (add): Entity " << this->name << " has no property " << name << endl; return; }
    prop = prop->copy();
    prop->value = value;
    properties[name].push_back( prop );
}

void VREntity::rem(VRPropertyPtr p) {
    string name = p->getName();
    if (properties.count(name)) {
        auto& v = properties[name];
        v.erase( remove(v.begin(), v.end(), p), v.end() );
    }
}

void VREntity::setVector(string name, vector<string> v, string type) {
    string v_name = this->name+"_"+name;
    set(name, v_name);
    if (auto o = ontology.lock()) o->addVectorInstance(v_name, type, v);
}

vector<VRPropertyPtr> VREntity::getValues(string name) {
    if (name != "" && properties.count(name)) return properties[name];
    vector<VRPropertyPtr> res;
    if (name == "") for (auto pv : properties) for (auto p : pv.second) res.push_back(p);
    return res;
}

VRPropertyPtr VREntity::getValue(string name) {
    auto props = getValues(name);
    if (props.size() == 0) return 0;
    return props[0];
}

vector<VRPropertyPtr> getProperties();

vector<string> VREntity::getAtPath(vector<string> path) {
    /*cout << "  get value at path ";
    for (auto p : path) cout << "/" << p;
    cout << endl;*/

    vector<string> res;
    if (path.size() == 2) {
        string m = path[1];
        auto prop = getProperty(m);
        //cout << "  get value of member " << m << " with id " << id << endl;
        if (!prop) return res;
        if (!properties.count(prop->getName())) return res;
        for (auto p : properties[prop->getName()]) res.push_back(p->value);
        return res;
    }

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
