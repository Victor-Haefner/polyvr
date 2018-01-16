#include "TrafficSimulator.h"

TrafficSimulator::TrafficSimulator() : roadSystem(), microsimulator(&roadSystem), mesosimulator(&roadSystem) {
    ;
}

RoadSystem* TrafficSimulator::getRoadSystem() {
    return &roadSystem;
}

void TrafficSimulator::tick() {
    ptime loopStart = timer.getTime();
    timer.tick();
    roadSystem.tick();
    mesosimulator.tick();
    microsimulator.tick();

    if ((timer.getTime() - loopStart).total_microseconds() < 100 * 1000 / timer.getTimeScale())
        usleep(100 * 1000 / timer.getTimeScale() - (timer.getTime() - loopStart).total_microseconds());
}
