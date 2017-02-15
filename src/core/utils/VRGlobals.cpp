#include "VRGlobals.h"

using namespace OSG;

VRGlobals::FPS::FPS(Int ms, Int fps, string s1, string s2) : ms(ms), fps(fps), statFPS(s1.c_str(), s2.c_str()) {}

void VRGlobals::FPS::update(VRTimer& t) {
    ms = t.stop();
    fps = round(1000.0/max(ms,1lu));
}

VRGlobals::Int VRGlobals::CURRENT_FRAME = 0;
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
