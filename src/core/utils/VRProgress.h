#ifndef VRPROGRESS_H_INCLUDED
#define VRPROGRESS_H_INCLUDED

#include <string>
#include <OpenSG/OSGConfig.h>
#include "core/utils/VRUtilsFwd.h"
#include "core/utils/VRFunctionFwd.h"

using namespace std;

OSG_BEGIN_NAMESPACE;

class VRProgress {
    public:
        enum Mode {
            CONSOLE_M = 0,
			WIDGET_M,
			GEOM_M,
			CALLBACK_M
        };

    private:
        string title;
        int max = 0;
		Mode mode = CONSOLE_M;
        int count = 0;
        double part = 0;
        VRUpdateCbWeakPtr callback;

    public:
		VRProgress(string title = "Progress", int max = 100, Mode m = CONSOLE_M);
        ~VRProgress();

        static VRProgressPtr create(string title = "Progress", int max = 100, Mode m = CONSOLE_M);

        void setCallback(VRUpdateCbPtr cb);
        float get();
        void set(float t);

        void setup(string title, int max, Mode m = CONSOLE_M);
        void update(int i = 1);
        void finish();
        void reset();
};

OSG_END_NAMESPACE;

#endif // VRPROGRESS_H_INCLUDED
