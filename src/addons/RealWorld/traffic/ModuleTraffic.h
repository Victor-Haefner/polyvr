#ifndef MODULETRAFFIC_H
#define MODULETRAFFIC_H

#include "../Modules/BaseModule.h"
#include "TrafficSimulation.h"
#include "core/utils/VRFunctionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class ModuleTraffic: public BaseModule {
    private:
        OldTrafficSimulation* simulation;
        VRThreadCbPtr threadFkt;

    public:
        ModuleTraffic();
        ~ModuleTraffic();

        OldTrafficSimulation *getTrafficSimulation();

        virtual void loadBbox(MapGrid::Box bbox);
        virtual void unloadBbox(MapGrid::Box bbox);

        void physicalize(bool b);

        OldTrafficSimulation *getSimulation();
};

OSG_END_NAMESPACE;

#endif // MODULETRAFFIC_H




