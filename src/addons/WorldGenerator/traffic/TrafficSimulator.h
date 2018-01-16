#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "RoadSystem.h"
#include "Microsimulator.h"
#include "Mesosimulator.h"

class TrafficSimulator {
    private:
        RoadSystem roadSystem; // The road system to operate on.
        Microsimulator microsimulator; // The simulator for micro-vehicle.
        Mesosimulator mesosimulator; // The simulator for meso-streets.

    public:
        TrafficSimulator();
        RoadSystem* getRoadSystem();
        void tick();
};

#endif // SIMULATOR_H
