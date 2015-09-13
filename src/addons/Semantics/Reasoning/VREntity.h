#ifndef VRENTITY_H_INCLUDED
#define VRENTITY_H_INCLUDED

#include "VROntologyUtils.h"
#include "VRConcept.h"

using namespace std;

struct VREntity : public VRNamedID {
    VRConcept* concept;
    map<int, vector<string> > properties;

    VREntity(string name, VRConcept* c);
    void set(string name, string value);
    void add(string name, string value);
    string toString();

    vector<string> getAtPath(vector<string> path);
};

#endif
