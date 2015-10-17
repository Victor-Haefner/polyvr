#ifndef TIMER_H
#define	TIMER_H

#include <GL/glut.h>
#include <map>
#include <string>

using namespace std;

class Timer {
    public:
        map<string, int> timers;

        void start(string name);
        void printTime(string name);
};

#endif	/* TIMER_H */

