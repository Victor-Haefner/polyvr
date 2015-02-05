#ifndef VRPROGRESS_H_INCLUDED
#define VRPROGRESS_H_INCLUDED

#include <string>
#include "core/utils/VRFunction.h"

using namespace std;

class VRProgress {
    public:
        enum Mode {
            CONSOLE,
            WIDGET,
            CALLBACK
        };

    private:
        string title;
        float N_100 = 0;
        Mode mode = CONSOLE;
        int j,k;
        VRFunction<int>* callback = 0;

    public:
        VRProgress(string title, int max, Mode m = CONSOLE);
        ~VRProgress();

        void setCallback(VRFunction<int>* cb);

        void update(int i = 1);
        void reset();
};

#endif // VRPROGRESS_H_INCLUDED
