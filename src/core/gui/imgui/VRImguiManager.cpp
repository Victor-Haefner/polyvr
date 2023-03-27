#include "VRImguiManager.h"

#include <iostream>
#include <thread>
#include <core/gui/VRGuiManager.h>

using namespace OSG;

VRImguiManager::VRImguiManager() {}
VRImguiManager::~VRImguiManager() {}

void VRImguiManager::setupCallbacks() {
    auto mgr = VRGuiSignals::get();
    mgr->addCallback("glutRenderUI", [&](VRGuiSignals::Options){ imgui.render(); return true; } );
    //addCallback("glutRenderGL", [&](Options){ imgui.renderGLArea(); return true; } );
    mgr->addResizeCallback("glutResizeUI", [&](int x, int y, int w, int h){ imgui.resizeUI({x,y,w,h}); return true; } );
    //addResizeCallback("glutResizeGL", [&](Surface s){ imgui.resizeGL(s); return true; } );
    mgr->addCallback("widgetResize", [&](VRGuiSignals::Options o){ imgui.onWidgetResize(o); return true; } );
    mgr->addResizeCallback("glutResize", [&](int x, int y, int w, int h){ imgui.resizeUI({x,y,w,h}); return true; } );
}

void VRImguiManager::initImgui() {
    auto sig = [&](string name, map<string,string> opts) -> bool { return VRGuiManager::trigger(name,opts); };
    auto sig2 = [&](string name, Surface s) -> bool { return VRGuiManager::triggerResize(name,s.x,s.y,s.width,s.height); };
    imgui.init(sig, sig2);
}
