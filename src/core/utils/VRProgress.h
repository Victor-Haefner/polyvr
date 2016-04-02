#ifndef VRPROGRESS_H_INCLUDED
#define VRPROGRESS_H_INCLUDED

#include <string>
#include "core/utils/VRUtilsFwd.h"
#include "core/utils/VRFunctionFwd.h"

using namespace std;

class VRProgress {
    public:
        enum Mode {
            CONSOLE_M,
			WIDGET_M,
			GEOM_M,
			CALLBACK_M
        };

    private:
        string title;
        float N_100 = 0;
		Mode mode = CONSOLE_M;
        int j,k;
        VRUpdateWeakPtr callback;

    public:
		VRProgress(string title = "Progress", int max = 100, Mode m = CONSOLE_M);
        ~VRProgress();

        static VRProgressPtr create();

        void setCallback(VRUpdatePtr cb);

        void setup(string title, int max, Mode m = CONSOLE_M);
        void update(int i = 1);
        void reset();
};

#endif // VRPROGRESS_H_INCLUDED
