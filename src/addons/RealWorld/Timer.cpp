#include "Timer.h"

void Timer::start(string name) {
    timers[name] = glutGet(GLUT_ELAPSED_TIME);
}

void Timer::printTime(string name) {
    int secs = glutGet(GLUT_ELAPSED_TIME) - timers[name];
    printf("TIME(%s)=%ds\n", name.c_str(), secs);
}
