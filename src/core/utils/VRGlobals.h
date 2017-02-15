#ifndef VRGLOBALS_H_INCLUDED
#define VRGLOBALS_H_INCLUDED

#include <OpenSG/OSGStatElemDesc.h>
#include <OpenSG/OSGStatIntElem.h>
#include <map>
#include "VRTimer.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGlobals {
    public:
        typedef unsigned long int Int;
        typedef OSG::StatElemDesc<OSG::StatIntElem> StatRate;

        struct FPS {
            Int ms;
            Int fps;
            StatRate statFPS;

            FPS(Int ms, Int fps, string s1, string s2);

            void update(VRTimer& t);
        };

    public:
        static Int CURRENT_FRAME;
        static FPS FRAME_RATE;
        static FPS GTK1_FRAME_RATE;
        static FPS WINDOWS_FRAME_RATE;
        static FPS GTK2_FRAME_RATE;
        static FPS RENDER_FRAME_RATE;
        static FPS SLEEP_FRAME_RATE;
        static FPS SWAPB_FRAME_RATE;
        static FPS SMCALLBACKS_FRAME_RATE;
        static FPS SETUP_FRAME_RATE;
        static FPS SCRIPTS_FRAME_RATE;
        static FPS PHYSICS_FRAME_RATE;
};

OSG_END_NAMESPACE;

#endif // VRGLOBALS_H_INCLUDED
