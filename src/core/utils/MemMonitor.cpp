#include "MemMonitor.h"
#include "system/VRSystem.h"

#include <thread>

/**
Define MEM_OVERRIDES to override the new and delete operators.
Then you can use MemMonitor::enable and MemMonitor::disable to track memory allocations between both calls.
*/

//#define MEM_OVERRIDES

bool MemMonitor::guard = 0;
MemMonitor* MemMonitor::instance = 0;


MemMonitor::MemMonitor() {
    threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
    cout << "Setup memory monitor" << endl;
}

MemMonitor::~MemMonitor() { cout << "Shutdown memory monitor, final count: " << std::dec << count << ", max allocated: " << std::dec << maxc << endl; }

void MemMonitor::add(void* p, size_t n) {
    if (guard) return;
    size_t tID = std::hash<std::thread::id>{}(std::this_thread::get_id());
    if (tID != threadID) return;

    //if (n == 1280) printBacktrace(); // OpenSG extending changelists..
    if (n == 1280) { // hack to work around changelists
        count -= 40;
        return;
    }

    guard = true;
    //cout << " allocate " << std::dec << n << " to " << p << endl;
    count += n;
    sizes[p] = n;
    maxc = max(maxc, count);
    guard = false;
}

void MemMonitor::sub(void* p) {
    if (guard) return;
    size_t tID = std::hash<std::thread::id>{}(std::this_thread::get_id());
    if (tID != threadID) return;

    if (!sizes.count(p)) {
        cout << "  delete " << p << ", not in sizes map!" << endl;
        return;
    }

    guard = true;
    //cout << "  delete " << p << " with " << std::dec << sizes[p] << endl;
    count -= sizes[p];
    guard = false;
}

void MemMonitor::enable() {
#ifndef MEM_OVERRIDES
    cout << "Warning in MemMonitor::enable, overrides not set, recompile with MEM_OVERRIDES in MemMonitor.cpp" << endl;
#endif // MEM_OVERRIDES

    guard = true;
    if (instance) delete instance;
    instance = new MemMonitor();
    guard = false;
}

void MemMonitor::disable() {
    guard = true;
    if (instance) delete instance;
    instance = 0;
    guard = false;
}

MemMonitor* MemMonitor::get() { return instance; }

#ifdef MEM_OVERRIDES
void* operator new(std::size_t n) noexcept {
    void* p = malloc(n);
    if (auto m = MemMonitor::get()) m->add(p, n);
    return p;
}

void* operator new[](std::size_t n) noexcept {
    void* p = malloc(n);
    if (auto m = MemMonitor::get()) m->add(p, n);
    return p;
}

void operator delete(void* p) noexcept {
    if (auto m = MemMonitor::get()) m->sub(p);
    free(p);
}

void operator delete[](void* p) noexcept {
    if (auto m = MemMonitor::get()) m->sub(p);
    free(p);
}
#endif
