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
        size_t max = 0;
		Mode mode = CONSOLE_M;
        size_t count = 0;
        double part = 0;
        VRAnimCbWeakPtr callback;
        VRTimerPtr timer;

    public:
		VRProgress(string title = "Progress", size_t max = 100, Mode m = CONSOLE_M);
        ~VRProgress();

        static VRProgressPtr create(string title = "Progress", size_t max = 100, Mode m = CONSOLE_M);

        void setCallback(VRAnimCbPtr cb);
        float get();
        size_t current();
        size_t left();
        void set(float t);

        void setup(string title, size_t max, Mode m = CONSOLE_M);
        void update(size_t i = 1);
        void finish();
        void reset();
};

OSG_END_NAMESPACE;

#endif // VRPROGRESS_H_INCLUDED
