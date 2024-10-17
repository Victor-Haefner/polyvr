#include "Thread.h"

#include <iostream>

Thread::Thread() {}

Thread::~Thread() {
    //cout << "~Thread " << name << endl;
    if (stopFlag) *stopFlag = false;
    if (t.joinable()) {
        if (doNothing) { /*cout << " ..ignore " << endl;*/ }
        else if (doDetach) { /*cout << " ..detach " << endl;*/ t.detach(); }
        else          { /*cout << " ..join "   << endl;*/ t.join(); }
    }
}

void Thread::swap(Thread& __t) noexcept {
    t.swap(__t.t);
    std::swap(name, __t.name);
    std::swap(stopFlag, __t.stopFlag);
    std::swap(doDetach, __t.doDetach);
}

bool Thread::joinable() { return t.joinable(); }
void Thread::join() { t.join(); }
void Thread::detach() { t.detach(); }

void Thread::onStopDetach() { doDetach = true; }
void Thread::onStopNothing() { doNothing = true; }

void Thread::sleepMilli(int ms) {
    this_thread::sleep_for(chrono::milliseconds(ms));
}
