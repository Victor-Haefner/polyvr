#ifndef VRTIMER_H_INCLUDED
#define VRTIMER_H_INCLUDED

#include <map>
#include <string>

class VRTimer {
    private:
        struct timer {
            double total = 0;
            double start = 0;
        };

        timer single;
        std::map<std::string, timer> timers;
        static std::map<std::string, timer> beacons;

    public:
        VRTimer();
        void start();
        double stop();
        void reset();

        void start(std::string t);
        double stop(std::string t);
        void print();

        static void emitBeacon(std::string);
        static double getBeacon(std::string);
};

#endif // VRTIMER_H_INCLUDED
