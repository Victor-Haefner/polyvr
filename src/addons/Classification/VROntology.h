#ifndef VRONTOLOGY_H_INCLUDED
#define VRONTOLOGY_H_INCLUDED

#include <string>
#include <map>
#include <vector>

using namespace std;

struct VRNamedID {
    string name;
    int ID;
    VRNamedID();
};

struct VRProperty : public VRNamedID {
    string value;
    string type;

    VRProperty(string name, string type);
};

struct VROntologyRule : public VRNamedID {
    string rule;
};

struct VRConcept : public VRNamedID {
    VRConcept* parent = 0;
    map<int, VRConcept*> children;
    map<int, VRProperty*> properties;

    VRConcept(string name);

    VRConcept* append(string name);
    void append(VRConcept* c);

    VRProperty* addProperty(string name, string type);
    void addProperty(VRProperty* p);

    int getPropertyID(string name);

    bool is_a(string concept);
};

struct VROntologyInstance : public VRNamedID {
    VRConcept* concept;
    map<int, vector<string>> properties;
    VROntologyInstance(string name, VRConcept* c);
    void set(string name, string value);
    void add(string name, string value);
    string toString();
};

struct VRTaxonomy {
    VRConcept* thing;
    VRTaxonomy();
    VRConcept* get(string name, VRConcept* p = 0);
};

struct VROntology {
    VRTaxonomy* taxonomy;
    map<int, VROntologyInstance*> instances;
    map<int, VROntologyRule*> rules;
    VROntology();
    void merge(VROntology* o);
    VROntologyInstance* addInstance(string concept, string name);
    vector<VROntologyInstance*> getInstances(string concept);
};



#endif // VRONTOLOGY_H_INCLUDED
