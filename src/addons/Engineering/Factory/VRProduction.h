#ifndef VRPRODUCTION_H_INCLUDED
#define VRPRODUCTION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include <vector>

#include "core/objects/VRObjectFwd.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "core/utils/VRName.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class FLogistics;
class FNetwork;

struct VRProcessFragment;
struct VRProcessRequirement;
struct VRProcessDependency;

struct VRProcessResult : VROntoID, VRName {
    string state;
    VRProcessResult(string name);
};

struct VRProcessDependency {
    vector<VRProcessFragment*> subprocess;
};

struct VRProcessAction : VROntoID, VRName {
    vector<VRProcessFragment*> subprocess;
};

struct VRProcessFragment : VROntoID, VRName {
    VRProcessAction* action = 0;
    vector<VRProcessDependency*> dependencies;
    vector<VRProcessResult*> results;

    VRProcessFragment(string name);
    VRProcessResult* addResult(string name);
};

struct VRProductionProcess : VROntoID, VRName {
    map<int, VRProcessFragment*> fragments;

    VRProductionProcess(string name);
    void addFragment(VRProcessFragment* f);
    string toString();
};

struct VRProduct : VROntoID, VRName {
    VROntologyPtr description;
    VRProduct(string name);
};

struct VRProductionMachine : VROntoID, VRName {
    VRGeometryPtr geo = 0;
    VROntologyPtr description;
    VRProductionMachine();
};

struct VRProductionJob {
    VRProduct* product = 0;
    VRProductionProcess* process = 0;
    VRProductionJob(VRProduct* p);
};

class VRProduction {
    private:
        VROntologyPtr description;
        map<int, VRProductionMachine*> machines;
        vector<VRProductionJob*> jobs;
        shared_ptr<FLogistics> intraLogistics;
        shared_ptr<FNetwork> network;
        bool running = false;
        int takt = 2000;
        int last_takt = 0;

    public:
        VRProduction();
        static shared_ptr<VRProduction> create();

        void addMachine(VRProductionMachine* pm, string machine, VRGeometryPtr m);
        VRProductionJob* queueJob(VRProduct* p, string prod);

        void setRate(float seconds);

        void start();
        void stop();
        void update();

        static VRObjectPtr test();
};

OSG_END_NAMESPACE;

#endif // VRPRODUCTION_H_INCLUDED
