#include "Thread.h"

#include <iostream>

Thread::Thread() {}

Thread::~Thread() {
    cout << "~Thread " << name << endl;
    //if (t.joinable()) { cout << " ..join " << endl; t.detach(); }
    if (stopFlag) *stopFlag = false;
    if (t.joinable()) { cout << " ..join " << endl; t.join(); }
}

bool Thread::joinable() { return t.joinable(); }
void Thread::join() { t.join(); }
void Thread::detach() { t.detach(); }

void Thread::sleepMilli(int ms) {
    this_thread::sleep_for(chrono::milliseconds(ms));
}
