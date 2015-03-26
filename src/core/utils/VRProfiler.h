#ifndef VRPROFILER_H_INCLUDED
#define VRPROFILER_H_INCLUDED

#include <list>
#include <map>

using namespace std;

class VRProfiler {
    public:
        struct Call {
            string name;
            int t0;
            int t1;
        };

        struct Frame {
            map<int, Call> calls;
        };

    private:
        list<Frame> frames;

        VRProfiler();

    public:
        static VRProfiler* get();

        int regStart(string name);
        void regStop(int ID);

        list<Frame> getFrames();

        void setHistoryLength();

        void swap();
};

#endif // VRPROFILER_H_INCLUDED
