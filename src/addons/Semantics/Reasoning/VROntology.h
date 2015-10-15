#ifndef VRONTOLOGY_H_INCLUDED
#define VRONTOLOGY_H_INCLUDED

#include "VRConcept.h"
#include "VROntologyRule.h"
#include "VREntity.h"

#include <string>
#include <map>
#include <vector>

using namespace std;

struct VROntology {
    VRConcept* thing = 0;
    map<int, VREntity*> instances;
    map<int, VROntologyRule*> rules;

    VROntology();

    void merge(VROntology* o);

    VRConcept* addConcept(string concept, string parent = "");
    VROntologyRule* addRule(string rule);
    VREntity* addInstance(string name, string concept);
    VREntity* addVectorInstance(string name, string concept, string x, string y, string z);

    VRConcept* getConcept(string name, VRConcept* p = 0);
    VREntity* getInstance(string instance);
    vector<VREntity*> getInstances(string concept);

    vector<VROntologyRule*> getRules();

    string answer(string question);
};



#endif // VRONTOLOGY_H_INCLUDED
