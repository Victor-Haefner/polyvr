#ifndef VRPROFILER_H_INCLUDED
#define VRPROFILER_H_INCLUDED

#include <list>
#include <map>
#include <boost/thread/mutex.hpp>

using namespace std;

class VRProfiler {
    public:
        struct Call {
            string name;
            int t0;
            int t1;
        };

        struct Frame {
            int t0, t1;
            bool running = true;
            map<int, Call> calls;
        };

    private:
        list<Frame> frames;
        Frame* current = 0;
        int ID = 0;
        int history = 100;
        bool active = true;

        boost::mutex mutex;

        int getTime();

        VRProfiler();

    public:
        static VRProfiler* get();

        void setActive(bool b);
        bool isActive();

        int regStart(string name);
        void regStop(int ID);

        list<Frame> getFrames();

        void setHistoryLength(int N);
        int getHistoryLength();

        void swap();
};

#endif // VRPROFILER_H_INCLUDED
