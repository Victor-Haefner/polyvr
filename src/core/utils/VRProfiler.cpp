#include "VRProfiler.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/VRGlobals.h"

#include <time.h>
#include <thread>
#include <iostream>

using namespace OSG;
using namespace std;

unsigned long getThreadId() {
    return hash<thread::id>{}( this_thread::get_id() );
}

VRProfiler* VRProfiler::get() {
    static VRProfiler* instance = new VRProfiler();
    return instance;
}

VRProfiler::VRProfiler() { swap(); }

int VRProfiler::regStart(string name) {
    if (!isActive()) return -1;
    if (current == 0) return -1;

    auto tID = getThreadId();
    if (!threadIDs.count(tID)) threadIDs[tID] = threadIDs.size();

    VRLock lock(mutex);
    ID++;
    Call c;
    c.name = name;
    c.t0 = getTime();
    c.cpu0 = getCPUTime();
    c.thread = threadIDs[tID];
    current->calls[ID] = c;
    return ID;
}

void VRProfiler::regStop(int ID) {
    if (ID < 0) return;
    if (!current || !current->calls.count(ID)) return;
    VRLock lock(mutex);
    current->calls[ID].t1 = getTime();
    current->calls[ID].cpu1 = getCPUTime();
}

void VRProfiler::setActive(bool b) { active = b; }
bool VRProfiler::isActive() { return active; }

list<VRProfiler::Frame> VRProfiler::getFrames() {
    VRLock lock(mutex);
    return frames;
}

VRProfiler::Frame VRProfiler::getFrame(int f) {
    VRLock lock(mutex);
    int i=0;
    for (auto fr : frames) {
        if (i == f) return (Frame)fr;
        i++;
    }
    return Frame();
}

void VRProfiler::swap() {
    if (!isActive()) return;

    VRLock lock(mutex);
    if (current) {
        current->t1 = getTime();
        current->cpu1 = getCPUTime();
    }
    Frame f;
    f.t0 = getTime();
    f.cpu0 = getCPUTime();
    f.fID = VRGlobals::CURRENT_FRAME;
    f.Nchanged = VRGlobals::NCHANGED;
    f.Ncreated = VRGlobals::NCREATED;
    frames.push_front(f);
    if (history <= (int)frames.size()) frames.pop_back();
    current = &frames.front();
}

void VRProfiler::setHistoryLength(int N) { history = N; }
int VRProfiler::getHistoryLength() { return history; }
