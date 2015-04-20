//#include "VRProduction.h"
#include "VRLogistics.h"
#include "core/objects/geometry/VRGeometry.h"

using namespace std;
using namespace OSG;

struct VRProcessFragment {
    void* operation = 0;
    void* in = 0;
    void* out = 0;
    int ID = 0;
};

struct VRProcess {
    map<int, VRProcessFragment*> fragments;

    void addFragment(VRProcessFragment* f) {
        fragments[f->ID] = f;
    }
};

struct VRProductionMachine {
    VRGeometry* geo = 0;
    VRProcessFragment* pfragment = 0;
    int ID = 0;
};

struct VRProductionJob {
    VRProcess* process = 0;
    ;
};

struct VRProduction {
    map<int, VRProductionMachine*> machines;
    vector<VRProcess*> processes;
    vector<VRProductionJob*> jobs;
    FLogistics* intraLogistics = 0;
    FNetwork* network = 0;

    VRProduction();
    void addMachine(VRGeometry* m);
    VRProcess* newProcess();
    VRProductionJob* queueJob(VRProcess* p);

    static VRProduction* test();
};

VRProduction::VRProduction() {
    intraLogistics = new FLogistics();
    network = intraLogistics->addNetwork();
}

void VRProduction::addMachine(VRGeometry* m) {
    auto pm = new VRProductionMachine();
    pm->geo = m;
    pm->pfragment = new VRProcessFragment();
    static int ID = 0;
    machines[ID] = pm;
    pm->ID = ID;
    FNode* n0 = network->addNode();
    for (auto n : network->getNodes()) if (n != n0) { n0->connect(n); n->connect(n0); }
    ID++;
}

VRProcess* VRProduction::newProcess() {
    auto p = new VRProcess();
    processes.push_back(p);
    return p;
}

VRProductionJob* VRProduction::queueJob(VRProcess* p) {
    auto j = new VRProductionJob();
    jobs.push_back(j);
    return j;
}


VRProduction* VRProduction::test() {
    auto machine = new VRGeometry("machine");
    machine->setPrimitive("Box", "1 2 1 1 1 1");


    auto p = new VRProduction();
    p->addMachine((VRGeometry*)machine->duplicate());
    p->addMachine((VRGeometry*)machine->duplicate());
    p->addMachine((VRGeometry*)machine->duplicate());
    p->addMachine((VRGeometry*)machine->duplicate());
    p->addMachine((VRGeometry*)machine->duplicate());

    auto proc = p->newProcess();
    p->queueJob(proc);

    return p;
}
