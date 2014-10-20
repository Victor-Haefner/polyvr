#ifndef TIMER_H
#define	TIMER_H

using namespace OSG;
using namespace std;

namespace realworld {
    class Timer {
    public:
        map<string, timeval*> timers;

        void start(string name) {
            timeval* t = new timeval();
            gettimeofday(t, NULL);
            timers[name] = t;
        }

        void printTime(string name) {
            timeval* t = new timeval();
            gettimeofday(t, NULL);

            timeval* tStart = timers[name];

            int secs(t->tv_sec - tStart->tv_sec);
            int usecs(t->tv_usec - tStart->tv_usec);

            if(usecs < 0)
            {
                --secs;
                usecs += 1000000;
            }

            int durationInMsecs = static_cast<int>(secs * 1000 + usecs / 1000.0 + 0.5);
            printf("TIME(%s)=%dms\n", name.c_str(), durationInMsecs);
        }
    };
}

#endif	/* TIMER_H */

