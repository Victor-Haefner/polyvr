#include "VRProfiler.h"

#include "core/scene/VRSceneManager.h"

#include <GL/glut.h>
#include <time.h>
#include <iostream>

using namespace OSG;

VRProfiler* VRProfiler::get() {
    static VRProfiler* instance = new VRProfiler();
    return instance;
}

VRProfiler::VRProfiler() { swap(); }

int VRProfiler::regStart(string name) {
    if (!isActive()) return -1;
    if (current == 0) return -1;

    boost::mutex::scoped_lock lock(mutex);
    ID++;
    Call c;
    c.name = name;
    c.t0 = getTime();
    current->calls[ID] = c;
    return ID;
}

void VRProfiler::regStop(int ID) {
    if (ID < 0) return;
    boost::mutex::scoped_lock lock(mutex);
    current->calls[ID].t1 = getTime();
}

int VRProfiler::getTime() {
    clock_t c = clock();
    return c;
}

void VRProfiler::setActive(bool b) { active = b; }

bool VRProfiler::isActive() {
    if (!active) return false;
    auto s = VRSceneManager::getCurrent();
    if (s == 0) return false;
    return true;
}

list<VRProfiler::Frame> VRProfiler::getFrames() {
    boost::mutex::scoped_lock lock(mutex);
    return frames;
}

void VRProfiler::swap() {
    if (!isActive()) return;

    boost::mutex::scoped_lock lock(mutex);
    if (current) current->t1 = getTime();
    if (current) cout << "swap profiler frame " << current->calls.size() << " " << current->t1-current->t0 << endl;
    Frame f;
    f.t0 = getTime();
    frames.push_front(f);
    if (history <= frames.size()) frames.pop_back();
    current = &frames.front();
}

void VRProfiler::setHistoryLength(int N) { history = N; }
int VRProfiler::getHistoryLength() { return history; }
