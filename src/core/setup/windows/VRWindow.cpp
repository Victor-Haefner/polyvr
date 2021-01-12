#include "VRWindow.h"
#ifndef WITHOUT_MTOUCH
#include "core/setup/devices/VRMultiTouch.h"
#endif
#include "core/setup/devices/VRMouse.h"
#include "core/setup/devices/VRKeyboard.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/rendering/VRRenderStudio.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRThreadManager.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/utils/VRProfiler.h"
#include "core/utils/xml.h"

using namespace OSG;

template<> string typeName(const VRWindow& o) { return "Window"; }

unsigned int VRWindow::active_window_count = 0;

VRWindow::VRWindow() : changeListStats("remote") {
    cout << "New Window" << endl;
    active_window_count++;
    string n = getName();
#ifndef WASM
    winThread = VRThreadCb::create("VRWindow", bind(&VRWindow::update, this, _1) );
    thread_id = VRSceneManager::get()->initThread(winThread,"window_"+n,true,0); // WASM crash, needed?
#endif
}

VRWindow::~VRWindow() {
    cout << " VRWindow::~VRWindow\n";
    if (auto sm = VRSceneManager::get())
        if (thread_id >= 0) sm->stopThread(thread_id);
    _win = NULL;
    active_window_count--;
}

VRWindowPtr VRWindow::create() { return VRWindowPtr(new VRWindow()); }
VRWindowPtr VRWindow::ptr() { return shared_from_this(); }

WindowMTRecPtr VRWindow::getOSGWindow() { return _win; }

void VRWindow::addView(VRViewPtr view) {
    views.push_back(view);
    view->setWindow(_win);
}

void VRWindow::remView(VRViewPtr view) {
    if (mouse) mouse->setViewport(0);
    for (unsigned int i=0;i<views.size();i++) {
        if (views[i].lock() != view) continue;
        views.erase(views.begin() + i);
        return;
    }
}

void VRWindow::setMSAA(string s) { msaa = s; }
string VRWindow::getMSAA() { return msaa; }

void VRWindow::stop() { stopping = true; }
void VRWindow::setAction(RenderActionRefPtr ract) { this->ract = ract; }
bool VRWindow::hasType(int i) { return (i == type); }
Vec2i VRWindow::getSize() { return Vec2i(width, height); }
void VRWindow::render(bool fromThread) { if(_win) _win->render(ract); }
void VRWindow::sync(bool fromThread) { ; }
void VRWindow::clear(Color3f c) { ; }

void VRWindow::resize(int w, int h) {
    if (_win->getWidth() == w && _win->getHeight() == h) return;

    width = w;
    height = h;
    _win->resize(w,h);
    for (auto vw : views) {
        if (auto v = vw.lock()) v->setSize(Vec2i(w,h));
    }
}

vector<VRViewPtr> VRWindow::getViews() {
    vector<VRViewPtr> res;
    for (auto v : views) if (auto r = v.lock()) res.push_back(r);
    return res;
}

// ----------------- clustering pitfalls
// clist->clear() will crash when an object is instantiated in main thread (VRGeometry::setMesh -> OSG::Window::registerGLObject)
// will also crash in OSG::RemoteAspect::sendSync

void VRWindow::update( weak_ptr<VRThread>  wt) {
#ifndef WASM
    auto t = wt.lock();
    do {
        t = wt.lock();
        BarrierRefPtr barrier = Barrier::get("PVR_rendering", true);
        auto appCL = t->appThread->getChangeList();
        auto clist = Thread::getCurrentChangeList();

        auto wait = [&]() {
            waitingAtBarrier = true;
#ifndef WASM
            int pID = VRProfiler::get()->regStart("window barrier "+getName());
#endif
            barrier->enter(active_window_count+1);
#ifndef WASM
            VRProfiler::get()->regStop(pID);
#endif
            waitingAtBarrier = false;
            if (stopping) return true;
            return false;
        };

        if (t->control_flag) {
            if (wait()) break;
            /** let the window manager initiate multi windows if necessary **/
            if (wait()) break;
            clist->merge(*appCL);
            //changeListStats.update();
            sync(true);
            clist->clear();
            if (wait()) break;
            render(true);
        }

        osgSleep(1);
    } while(t && t->control_flag);
    cout << "VRWindow::update done" << endl;
#endif
}

bool VRWindow::isWaiting() { return waitingAtBarrier; }

bool VRWindow::isActive() { return active; }
void VRWindow::setActive(bool b) { active = b; }

bool VRWindow::hasContent() { return content; }
void VRWindow::setContent(bool b) { content = b; }

void VRWindow::setMouse(VRMousePtr m) { mouse = m; }
VRMousePtr VRWindow::getMouse() { return mouse; }
void VRWindow::setMultitouch(VRMultiTouchPtr m) { multitouch = m; }
VRMultiTouchPtr VRWindow::getMultitouch() { return multitouch; }

void VRWindow::setKeyboard(VRKeyboardPtr k) { keyboard = k; }
VRKeyboardPtr VRWindow::getKeyboard() { return keyboard; }

void VRWindow::save(XMLElementPtr node) {
    node->setAttribute("active", toString(active).c_str());
    node->setAttribute("type", toString(type).c_str());
    node->setAttribute("width", toString(width).c_str());
    node->setAttribute("height", toString(height).c_str());
    node->setAttribute("name", getName().c_str());
    node->setAttribute("msaa", msaa.c_str());
    if (mouse) node->setAttribute("mouse", mouse->getName().c_str());
#ifndef WITHOUT_MTOUCH
    else if (multitouch) node->setAttribute("mouse", multitouch->getName().c_str());
#endif
    else node->setAttribute("mouse", "None");
    if (keyboard) node->setAttribute("keyboard", keyboard->getName().c_str());
    else node->setAttribute("keyboard", "None");

    XMLElementPtr vn;
    for (auto wv : views) {
        if (auto v = wv.lock()) {
            vn = node->addChild("View");
            v->save(vn);
        }
    }
}

void VRWindow::load(XMLElementPtr node) {
    active = toValue<bool>( node->getAttribute("active") );
    type = toInt( node->getAttribute("type") );
    width = toInt( node->getAttribute("width") );
    height = toInt( node->getAttribute("height") );
    name = node->getAttribute("name");
    if (node->hasAttribute("msaa")) msaa = node->getAttribute("msaa");

    for (auto el : node->getChildren()) {
        if (!el) continue;
        if (el->getName() != "View") continue;
        int i = VRSetup::getCurrent()->addView(name);
        VRViewPtr v = VRSetup::getCurrent()->getView(i);
        addView(v);
        v->load(el);
    }

    string _mouse = node->getAttribute("mouse");
    if (_mouse != "None") {
        auto dev = VRSetup::getCurrent()->getDevice(_mouse);
        mouse = dynamic_pointer_cast<VRMouse>( dev );
        if (views.size() > 0 && mouse) if (auto v = views[0].lock()) mouse->setViewport(v);
#ifndef WITHOUT_MTOUCH
        multitouch = dynamic_pointer_cast<VRMultiTouch>(dev);
        if (multitouch) multitouch->setWindow(ptr());
#endif
    }

    if (node->hasAttribute("keyboard") != 0) {
        string _keyboard = node->getAttribute("keyboard");
        if (_keyboard != "None") {
            keyboard = dynamic_pointer_cast<VRKeyboard>( VRSetup::getCurrent()->getDevice(_keyboard) );
        }
    }
}

