/**
 Written by Sebastian Friebe in SS14.
 Until SS16 questions are possible at
 sebastian.friebe@student.kit.edu
 */


#include "RoadSystem.h"


#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WITH_GUI
#include <QApplication>
#include <boost/thread.hpp>
#include "qtgui/window.h"
#endif // WITH_GUI

#include "TrafficSimulator.h"

#ifdef WITH_GUI
int main(int argc, char* argv[])
#else
int main()
#endif
{
    TrafficSimulator sim;
    timer.setTimeScale(1);

#ifdef WITH_GUI

	QApplication app(argc, argv);
	Window window(&sim);
	window.show();

    boost::thread simulation(&TrafficSimulator::mainLoop, &sim);

    app.exec();

    sim.setState(TrafficSimulator::STOP);
    simulation.join();

#else

    sim.mainLoop();

#endif

    return 0;
}
