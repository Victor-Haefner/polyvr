#ifndef TIMER_H
#define	TIMER_H

using namespace OSG;
using namespace std;

#include <GL/glut.h>

namespace realworld {
    class Timer {
    public:
        map<string, int> timers;

        void start(string name) {
			timers[name] = glutGet(GLUT_ELAPSED_TIME);
        }

        void printTime(string name) {
			int secs = glutGet(GLUT_ELAPSED_TIME) - timers[name];
			printf("TIME(%s)=%ds\n", name.c_str(), secs);
        }
    };
}

#endif	/* TIMER_H */

