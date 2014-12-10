#ifndef VRTIMER_H_INCLUDED
#define VRTIMER_H_INCLUDED

#include <map>
#include <string>

class VRTimer {
    private:
        struct timer {
            int total = 0;
            int start = 0;
        };

        timer single;
        std::map<std::string, timer> timers;
        static std::map<std::string, timer> beacons;

    public:
        void start();
        int stop();

        void start(std::string t);
        int stop(std::string t);
        void print();

        static void emitBeacon(std::string);
};

#endif // VRTIMER_H_INCLUDED
