#include "TrafficSimulator.h"

TrafficSimulator::TrafficSimulator() : roadSystem(), microsimulator(&roadSystem), mesosimulator(&roadSystem), state(PAUSE) {
    ;
}

void TrafficSimulator::setState(const STATE newState) {
    state = newState;
    if (state == PAUSE) {
        timer.stop();
        cout << "Simulator is now paused.\n";
    } else if (state == RUN) {
        timer.start();
        cout << "Simulator is now running.\n";
    }
}

TrafficSimulator::STATE TrafficSimulator::getState() const {
    return state;
}

RoadSystem* TrafficSimulator::getRoadSystem() {
    return &roadSystem;
}

void TrafficSimulator::mainLoop() {
        cout << "Simulator has started main loop.\n";
        while (state != STOP) {
            ptime loopStart = timer.getTime();
            timer.tick();
            if (state != PAUSE) {
                roadSystem.tick();
                mesosimulator.tick();
                microsimulator.tick();
            }

            if ((timer.getTime() - loopStart).total_microseconds() < 100 * 1000 / timer.getTimeScale())
                usleep(100 * 1000 / timer.getTimeScale() - (timer.getTime() - loopStart).total_microseconds());
        }
        cout << "Simulator will now quit.\n";
}
