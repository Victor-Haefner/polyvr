#include "VREntity.h"
#include "VRProperty.h"

#include <iostream>

VREntity::VREntity(string name, VRConceptPtr c) {
    this->name = name;
    concept = c;
}

VREntityPtr VREntity::create(string name, VRConceptPtr c) { return VREntityPtr( new VREntity(name, c) ); }

void VREntity::setConcept(VRConceptPtr c) { concept = c; }

void VREntity::set(string name, string value) {
    if (!properties.count(name)) { add(name, value); return; }
    auto prop = concept->getProperty(name);
    if (!prop) { cout << "Entity " << this->name << " has no property " << name << endl; return; }
    properties[prop->type][0]->value = value;
}

void VREntity::add(string name, string value) {
    auto prop = concept->getProperty(name);
    if (!prop) { cout << "Entity " << this->name << " has no property " << name << endl; return; }
    prop = prop->copy();
    prop->value = value;
    properties[prop->type].push_back( prop );
}

vector<VRPropertyPtr> VREntity::getProperties(string name) {
    if (name != "" && properties.count(name)) return properties[name];
    vector<VRPropertyPtr> res;
    for (auto pv : properties) for (auto p : pv.second) res.push_back(p);
    return res;
}

vector<string> VREntity::getAtPath(vector<string> path) {
    /*cout << "  get value at path ";
    for (auto p : path) cout << "/" << p;
    cout << endl;*/

    vector<string> res;

    if (path.size() == 2) {
        string m = path[1];
        auto prop = concept->getProperty(m);
        //cout << "  get value of member " << m << " with id " << id << endl;
        if (!prop) return res;
        if (!properties.count(prop->type)) return res;
        for (auto p : properties[prop->type]) res.push_back(p->value);
        return res;
    }

    return res;
}

string VREntity::toString() {
    string data = "Instance " + name + " of type " + concept->name;
    data += " with properties:";
    for (auto p : properties) {
        for (auto sp : p.second) {
            data += " "+sp->name+"("+sp->type+")="+sp->value;
        }
    }
    return data;
}
