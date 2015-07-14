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
    VROntologyRule(string rule);
};

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

struct VREntity : public VRNamedID {
    VRConcept* concept;
    map<int, vector<string> > properties;
    VREntity(string name, VRConcept* c);
    void set(string name, string value);
    void add(string name, string value);
    string toString();

    vector<string> getAtPath(vector<string> path);
};

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
