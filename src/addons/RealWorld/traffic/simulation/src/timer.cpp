#include "timer.h"

Timer timer;


Timer::Timer()
    : lastTick(microsec_clock::universal_time()), now(lastTick), delta(), running(true), timescale(1) {

}

void Timer::start() {
    running = true;
}

bool Timer::isRunning() {
    return running;
}

void Timer::stop() {
    running = false;
}

void Timer::tick() {

    delta = milliseconds(std::min((microsec_clock::universal_time() - lastTick).total_milliseconds() * timescale, 1000.0));
    lastTick = microsec_clock::universal_time();

    if (running) {
        now += delta;
    }
}

void Timer::setTimeScale(const double scale) {
    if (scale > 0)
        timescale = scale;
}

double Timer::getTimeScale() const {
    return timescale;
}

time_duration Timer::getDelta() {
    return delta;
}

ptime Timer::getTime() {
    return now;
}
