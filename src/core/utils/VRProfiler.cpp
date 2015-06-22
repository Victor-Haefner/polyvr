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
    //int c = glutGet(GLUT_ELAPSED_TIME);
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

VRProfiler::Frame VRProfiler::getFrame(int f) {
    boost::mutex::scoped_lock lock(mutex);
    int i=0;
    for (auto fr : frames) {
        if (i == f) return (Frame)fr;
        i++;
    }
    return Frame();
}

void VRProfiler::swap() {
    if (!isActive()) return;

    boost::mutex::scoped_lock lock(mutex);
    if (current) current->t1 = getTime();
    Frame f;
    f.t0 = getTime();
    frames.push_front(f);
    if (history <= (int)frames.size()) frames.pop_back();
    current = &frames.front();
}

void VRProfiler::setHistoryLength(int N) { history = N; }
int VRProfiler::getHistoryLength() { return history; }
