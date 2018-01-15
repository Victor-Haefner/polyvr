#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "RoadSystem.h"
#include "TrafficSimulator.h"
#include "Traffic.h"

void startTraffic(void) {
	TrafficSimulator sim;
	timer.setTimeScale(1);
	sim.mainLoop();
}
