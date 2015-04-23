#ifndef VRPRODUCTION_H_INCLUDED
#define VRPRODUCTION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include <vector>

class FLogistics;
class FNetwork;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGeometry;

struct VRNamedID {
    string name;
    int ID;
    VRNamedID();
};

struct VRProperty : public VRNamedID {
    string value;
};

struct VROntologyRule : public VRNamedID {
    string rule;
};

struct VRConcept : public VRNamedID {
    VRConcept* parent;
    map<int, VRConcept*> children;

    map<int, VRProperty*> properties;

    VRConcept(string name);

    VRConcept* append(string name);
    void append(VRConcept* c);
};

struct VROntologyInstance : public VRNamedID {
    VRConcept* concept;
    map<int, string> properties;
    VROntologyInstance(string name, VRConcept* c);
    void set(string name, string value);
    void set(string name, VROntologyInstance* i);
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
};


struct VRProcessFragment : VRNamedID {
    void* operation = 0;
    void* in = 0;
    void* out = 0;
};

struct VRProcess {
    map<int, VRProcessFragment*> fragments;

    void addFragment(VRProcessFragment* f);
};

struct VRProduct {
    VROntology* description;
    VRProduct();
};

struct VRProductionMachine : VRNamedID {
    VRGeometry* geo = 0;
    VROntology* description;
    VRProductionMachine();
};

struct VRProductionJob {
    VRProcess* process = 0;
    VRProductionJob(VRProcess* p);
};

class VRProduction {
    private:
        map<int, VRProductionMachine*> machines;
        vector<VRProcess*> processes;
        vector<VRProductionJob*> jobs;
        FLogistics* intraLogistics = 0;
        FNetwork* network = 0;

    public:
        VRProduction();

        VRProductionMachine* addMachine(VRGeometry* m);
        VRProcess* getProcess(VRProduct* p);
        VRProductionJob* queueJob(VRProcess* p);

        void setRate(float seconds);

        void start();
        void stop();

        static VRProduction* test();
};

OSG_END_NAMESPACE;

#endif // VRPRODUCTION_H_INCLUDED
