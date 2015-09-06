#include "VREntity.h"

#include <iostream>

void VREntity::set(string name, string value) {
    int id = concept->getPropertyID(name);
    if (id < 0) return;

    if (!properties.count(id)) { add(name, value); return; }

    properties[id][0] = value;
}

void VREntity::add(string name, string value) {
    int id = concept->getPropertyID(name);
    if (id < 0) return;

    if (!properties.count(id)) properties[id] = vector<string>();

    properties[id].push_back(value);
}

VREntity::VREntity(string name, VRConcept* c) {
    this->name = name;
    concept = c;
}

vector<string> VREntity::getAtPath(vector<string> path) {
    cout << "  get value at path ";
    for (auto p : path) cout << "/" << p;
    cout << endl;

    vector<string> res;

    if (path.size() == 2) {
        string m = path[1];
        int id = concept->getPropertyID(m);
        cout << "  get value of member " << m << " with id " << id << endl;
        if (id < 0) return res;
        if (!properties.count(id)) return res;
        return properties[id];
    }

    return res;
}

string VREntity::toString() {
    string data = "Instance " + name + " of type " + concept->name;
    data += " with properties:";
    for (auto p : properties) {
        for (auto sp : p.second) {
            data += " "+concept->properties[p.first]->name+"="+sp;
        }
    }
    return data;
}
