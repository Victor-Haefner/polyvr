#ifndef VRONTOLOGY_H_INCLUDED
#define VRONTOLOGY_H_INCLUDED

#include "VRConcept.h"
#include "VROntologyRule.h"
#include "VREntity.h"

#include <string>
#include <map>
#include <vector>
#include <memory>

using namespace std;

struct VROntology;
typedef shared_ptr<VROntology> VROntologyPtr;

struct VROntology : public std::enable_shared_from_this<VROntology> {
    VRConceptPtr thing = 0;
    map<int, VREntityPtr> instances;
    map<string, VRConceptWeakPtr> concepts;
    map<int, VROntologyRulePtr> rules;

    VROntology();
    static VROntologyPtr create();

    void merge(VROntologyPtr o);
    VROntologyPtr copy();

    void addConcept(VRConceptPtr);
    void addInstance(VREntityPtr);

    VRConceptPtr addConcept(string concept, string parent = "");
    VROntologyRulePtr addRule(string rule);
    VREntityPtr addInstance(string name, string concept);
    VREntityPtr addVectorInstance(string name, string concept, string x, string y, string z);

    vector<VRConceptPtr> getConcepts();
    VRConceptPtr getConcept(string name, VRConceptPtr p = 0);
    VREntityPtr getInstance(string instance);
    vector<VREntityPtr> getInstances(string concept);

    vector<VROntologyRulePtr> getRules();

    string answer(string question);

    void open(string path);

    string toString();
};



#endif // VRONTOLOGY_H_INCLUDED
