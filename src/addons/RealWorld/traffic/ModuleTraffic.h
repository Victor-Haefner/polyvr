#ifndef MODULETRAFFIC_H
#define MODULETRAFFIC_H

#include "../Modules/BaseModule.h"
#include "TrafficSimulation.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class ModuleTraffic: public BaseModule {
    private:
        TrafficSimulation* simulation;

    public:
        ModuleTraffic();
        ~ModuleTraffic();

        TrafficSimulation *getTrafficSimulation();

        virtual void loadBbox(AreaBoundingBox* bbox);
        virtual void unloadBbox(AreaBoundingBox* bbox);

        void physicalize(bool b);

        TrafficSimulation *getSimulation();
};

OSG_END_NAMESPACE;

#endif // MODULETRAFFIC_H




