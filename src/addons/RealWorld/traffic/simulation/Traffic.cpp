#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WITH_GUI
#include <QApplication>
#include <boost/thread.hpp>
#include "src/qtgui/window.h"
#endif // WITH_GUI

#include "src/RoadSystem.h"
#include "src/TrafficSimulator.h"

#include "Traffic.h"

void startTraffic(void) {
	TrafficSimulator sim;
	timer.setTimeScale(1);

#ifdef WITH_GUI

	QApplication app(0, 0);
	Window window(&sim);
	window.show();

    boost::thread simulation(&TrafficSimulator::mainLoop, &sim);

    app.exec();

    sim.setState(TrafficSimulator::STOP);
    simulation.join();

#else

    sim.mainLoop();

#endif
}
