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

#include "TrafficSimulator.h"

int main() {
    TrafficSimulator sim;
    timer.setTimeScale(1);
    sim.mainLoop();
    return 0;
}
