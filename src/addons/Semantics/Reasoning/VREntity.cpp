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
    if (!prop) { cout << "Warning (set): Entity " << this->name << " has no property " << name << endl; return; }
    properties[name][0]->value = value;
    // TODO: warn if vector size bigger 1
}

void VREntity::add(string name, string value) {
    string pconcept; if (auto p = concept->parent.lock()) pconcept = p->name;
    //cout << "VREntity::add " << name << " " << value << " " << concept->name << " " << pconcept << endl;
    auto prop = concept->getProperty(name);
    if (!prop) { cout << "Warning (add): Entity " << this->name << " has no property " << name << endl; return; }
    prop = prop->copy();
    prop->value = value;
    properties[name].push_back( prop );
}

vector<VRPropertyPtr> VREntity::getProperties(string name) {
    if (name != "" && properties.count(name)) return properties[name];
    vector<VRPropertyPtr> res;
    if (name == "") for (auto pv : properties) for (auto p : pv.second) res.push_back(p);
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
        if (!properties.count(prop->name)) return res;
        for (auto p : properties[prop->name]) res.push_back(p->value);
        return res;
    }

    return res;
}

string VREntity::toString() {
    string data = "Instance " + name;
    if (concept) data += " of type " + concept->name;
    data += " with properties:";
    for (auto p : properties) {
        for (auto sp : p.second) {
            data += " "+sp->name+"("+sp->type+")="+sp->value;
        }
    }
    return data;
}
