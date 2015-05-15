#ifndef VRPRODUCTION_H_INCLUDED
#define VRPRODUCTION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include <vector>

#include "addons/Classification/VROntology.h"

class FLogistics;
class FNetwork;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObject;
class VRGeometry;

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
    VRProduct* product = 0;
    VRProcess* process = 0;
    VRProductionJob(VRProduct* p);
};

class VRProduction {
    private:
        map<int, VRProductionMachine*> machines;
        vector<VRProductionJob*> jobs;
        FLogistics* intraLogistics = 0;
        FNetwork* network = 0;
        bool running = false;
        int takt = 2000;
        int last_takt = 0;

    public:
        VRProduction();

        VRProductionMachine* addMachine(VRGeometry* m);
        VRProductionJob* queueJob(VRProduct* p);

        void setRate(float seconds);

        void start();
        void stop();
        void update();

        static VRObject* test();
};

OSG_END_NAMESPACE;

#endif // VRPRODUCTION_H_INCLUDED
