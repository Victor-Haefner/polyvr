#ifndef MODULETRAFFIC_H
#define MODULETRAFFIC_H

#include "../Modules/BaseModule.h"
#include "TrafficSimulation.h"
#include "core/utils/VRFunctionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class ModuleTraffic: public BaseModule {
    private:
        TrafficSimulation* simulation;
        VRThreadCbPtr threadFkt;

    public:
        ModuleTraffic();
        ~ModuleTraffic();

        TrafficSimulation *getTrafficSimulation();

        virtual void loadBbox(MapGrid::Box bbox);
        virtual void unloadBbox(MapGrid::Box bbox);

        void physicalize(bool b);

        TrafficSimulation *getSimulation();
};

OSG_END_NAMESPACE;

#endif // MODULETRAFFIC_H




