#ifndef MEMMONITOR_H_INCLUDED
#define MEMMONITOR_H_INCLUDED

#include <iostream>
#include <map>

using namespace std;

class MemMonitor {
    private:
        static bool guard;
        static MemMonitor* instance;

        size_t threadID = 0;
        size_t count = 0;
        size_t maxc = 0;
        map<void*, size_t> sizes;

    public:
        MemMonitor();
        ~MemMonitor();

        void add(void* p, size_t n);
        void sub(void* p);

        static void enable();
        static void disable();

        static MemMonitor* get();
};

#endif // MEMMONITOR_H_INCLUDED
