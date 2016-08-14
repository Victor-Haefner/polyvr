#ifndef VRONTOLOGY_H_INCLUDED
#define VRONTOLOGY_H_INCLUDED

#include "VRConcept.h"
#include "VROntologyRule.h"
#include "VREntity.h"
#include "../VRSemanticsFwd.h"

#include <string>
#include <map>
#include <vector>
#include <memory>

using namespace std;
OSG_BEGIN_NAMESPACE;

struct VROntology : public std::enable_shared_from_this<VROntology> {
    static map<string, VROntologyPtr> library;
    static void setupLibrary();

    VRConceptPtr thing = 0;
    map<int, VREntityPtr> instances;
    map<string, VRConceptWeakPtr> concepts;
    map<int, VROntologyRulePtr> rules;

    VROntology();
    static VROntologyPtr create();

    void merge(VROntologyPtr o);
    VROntologyPtr copy();

    void addConcept(VRConceptPtr c);
    void remConcept(VRConceptPtr c);
    void renameConcept(VRConceptPtr c, string newName);
    void addInstance(VREntityPtr e);

    VRConceptPtr addConcept(string concept, string parent = "");
    VROntologyRulePtr addRule(string rule);
    VREntityPtr addInstance(string name, string concept);
    VREntityPtr addVectorInstance(string name, string concept, string x, string y, string z);
    VREntityPtr addVectorInstance(string name, string concept, vector<string> val);

    vector<VRConceptPtr> getConcepts();
    VRConceptPtr getConcept(string name);
    VREntityPtr getInstance(string instance);
    vector<VREntityPtr> getInstances(string concept);

    vector<VROntologyRulePtr> getRules();

    void open(string path);
    void addModule(string mod);
    string toString();
};

OSG_END_NAMESPACE;

#endif // VRONTOLOGY_H_INCLUDED
