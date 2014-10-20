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

        std::map<std::string, timer> timers;

    public:
        void start(std::string t);
        void stop(std::string t);
        void print();
};

#endif // VRTIMER_H_INCLUDED
