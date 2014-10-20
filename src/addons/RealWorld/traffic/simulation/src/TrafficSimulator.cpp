#include "TrafficSimulator.h"

#include <fstream>
#include <boost/thread.hpp>

#ifdef FILE_LOADING
void fileLoaderThread(NetworkInterface* network, Value v) {

    StyledWriter w;
    Value res = network->handlePostRequest(v);
    if (!res.isNull())
        cout << "Error while loading from file: " << w.write(res);
}
#endif

TrafficSimulator::TrafficSimulator()
    : roadSystem(), microsimulator(&roadSystem), mesosimulator(&roadSystem), network(), state(PAUSE) {

    network.start();

#ifdef FILE_LOADING
    ifstream file("postLog.json");
    string str;
    getline(file, str);
    Reader r;
    Value v;
    while (!str.empty()) {
        if (r.parse(str, v))
            boost::thread a(&fileLoaderThread, &network, v);
        else
            cout << "Error parsing file line.\n";
        getline(file, str);
    }
#endif
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
            network.applyChanges(this);
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
