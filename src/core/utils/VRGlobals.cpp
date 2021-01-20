#include "VRGlobals.h"

using namespace OSG;

VRGlobals::FPS::FPS(Int ms, Int fps, string s1, string s2) : ms(ms), fps(fps), statFPS(s1.c_str(), s2.c_str()) {}

void VRGlobals::FPS::update(VRTimer& t) {
    ms = t.stop();
    fps = round(1000.0/max(ms,1lu));

    static const int N = 60;
    if (history.size() < N) history.push_back(ms);
    else { history[pointer] = ms; pointer++; }
    if (pointer >= N) pointer = 0;

    /*int MS = 0;
    for (auto m : history) MS += m;
    mfps = round(1000.0*N/max(MS,1));*/

    Int mi = 1e6;
    Int ma = 1;
    for (auto m : history) {
        ma = max(ma, m);
        mi = max(min(mi, m),1ul);
    }
    min_fps = round(1000.0/ma);
    max_fps = round(1000.0/mi);
}

VRGlobals::Int VRGlobals::CURRENT_FRAME = 0;
VRGlobals::Int VRGlobals::NCHANGED = 0;
VRGlobals::Int VRGlobals::NCREATED = 0;
VRGlobals::FPS VRGlobals::FRAME_RATE(0,0,"statFPS", "PolyVR framerate");
VRGlobals::FPS VRGlobals::WINDOWS_FRAME_RATE(0,0,"statWinFPS", "PolyVR windows framerate");
VRGlobals::FPS VRGlobals::RENDER_FRAME_RATE(0,0,"statRenderFPS", "PolyVR rendering framerate");
VRGlobals::FPS VRGlobals::SLEEP_FRAME_RATE(0,0,"statSleepFPS", "PolyVR sleep framerate");
VRGlobals::FPS VRGlobals::SWAPB_FRAME_RATE(0,0,"statSwapbFPS", "PolyVR rendering swap buffer framerate");
VRGlobals::FPS VRGlobals::SCRIPTS_FRAME_RATE(0,0,"statScriptFPS", "PolyVR scripts framerate");
VRGlobals::FPS VRGlobals::PHYSICS_FRAME_RATE(2,500,"statPhysFPS", "PolyVR physics framerate");  /** TODO magic start number **/
VRGlobals::FPS VRGlobals::GTK1_FRAME_RATE(0,0,"statGtk1FPS", "GTK framerate");
VRGlobals::FPS VRGlobals::GTK2_FRAME_RATE(0,0,"statGtk2FPS", "GTK framerate");
VRGlobals::FPS VRGlobals::SMCALLBACKS_FRAME_RATE(0,0,"statSMCFPS", "Scene manager callbacks framerate");
VRGlobals::FPS VRGlobals::SETUP_FRAME_RATE(0,0,"statSetupFPS", "Setup devices framerate");
VRGlobals::FPS VRGlobals::UPDATE_LOOP1(0,0,"statLoop1FPS", "Main update loop point 1");
VRGlobals::FPS VRGlobals::UPDATE_LOOP2(0,0,"statLoop2FPS", "Main update loop point 2");
VRGlobals::FPS VRGlobals::UPDATE_LOOP3(0,0,"statLoop3FPS", "Main update loop point 3");
VRGlobals::FPS VRGlobals::UPDATE_LOOP4(0,0,"statLoop4FPS", "Main update loop point 4");
VRGlobals::FPS VRGlobals::UPDATE_LOOP5(0,0,"statLoop5FPS", "Main update loop point 5");
VRGlobals::FPS VRGlobals::UPDATE_LOOP6(0,0,"statLoop6FPS", "Main update loop point 6");
VRGlobals::FPS VRGlobals::UPDATE_LOOP7(0,0,"statLoop7FPS", "Main update loop point 7");
