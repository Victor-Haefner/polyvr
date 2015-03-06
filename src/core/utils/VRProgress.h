#ifndef VRPROGRESS_H_INCLUDED
#define VRPROGRESS_H_INCLUDED

#include <string>
#include "core/utils/VRFunction.h"

using namespace std;

class VRProgress {
    public:
        enum Mode {
            CONSOLE_M,
			WIDGET_M,
			CALLBACK_M
        };

    private:
        string title;
        float N_100 = 0;
		Mode mode = CONSOLE_M;
        int j,k;
        VRFunction<int>* callback = 0;

    public:
		VRProgress(string title, int max, Mode m = CONSOLE_M);
        ~VRProgress();

        void setCallback(VRFunction<int>* cb);

        void update(int i = 1);
        void reset();
};

#endif // VRPROGRESS_H_INCLUDED
