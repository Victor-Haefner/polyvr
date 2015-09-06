#ifndef VRCONCEPT_H_INCLUDED
#define VRCONCEPT_H_INCLUDED

#include "VROntologyUtils.h"
#include "VROntologyRule.h"
#include "VRProperty.h"

#include <map>
#include <vector>

using namespace std;

struct VRConcept : public VRNamedID {
    VRConcept* parent = 0;
    map<int, VRConcept*> children;
    map<int, VRProperty*> properties;
    vector<VROntologyRule*> rules;

    VRConcept(string name);

    VRConcept* append(string name);
    void append(VRConcept* c);

    VRProperty* addProperty(string name, string type);
    void addProperty(VRProperty* p);

    int getPropertyID(string name);

    bool is_a(string concept);
};

#endif
