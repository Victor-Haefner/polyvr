#include "VRWindow.h"
#include "core/setup/devices/VRMultiTouch.h"
#include "core/setup/devices/VRMouse.h"
#include "core/setup/devices/VRKeyboard.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/rendering/VRRenderStudio.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRThreadManager.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/utils/VRProfiler.h"
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

unsigned int VRWindow::active_window_count = 0;

VRWindow::VRWindow() {
    active_window_count++;
    string n = getName();
    winThread = VRThreadCb::create("VRWindow", boost::bind(&VRWindow::update, this, _1) );
    thread_id = VRSceneManager::get()->initThread(winThread,"window_"+n,true,0);
}

VRWindow::~VRWindow() {
    cout << " VRWindow::~VRWindow\n";
    if (auto sm = VRSceneManager::get()) sm->stopThread(thread_id);
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
    for (uint i=0;i<views.size();i++) {
        if (views[i].lock() != view) continue;
        views.erase(views.begin() + i);
        return;
    }
}

void VRWindow::stop() { stopping = true; }
void VRWindow::setAction(RenderActionRefPtr ract) { this->ract = ract; }
bool VRWindow::hasType(int i) { return (i == type); }
Vec2i VRWindow::getSize() { return Vec2i(width, height); }
void VRWindow::render(bool fromThread) { if(_win) _win->render(ract); }
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

void VRWindow::update( weak_ptr<VRThread>  wt) {
    auto t = wt.lock();
    do {
        t = wt.lock();
        BarrierRefPtr barrier = Barrier::get("PVR_rendering", true);
        auto appCL = t->appThread->getChangeList();
        auto clist = Thread::getCurrentChangeList();

        auto wait = [&]() {
            waitingAtBarrier = true;
            int pID = VRProfiler::get()->regStart("window barrier "+getName());
            barrier->enter(active_window_count+1);
            VRProfiler::get()->regStop(pID);
            waitingAtBarrier = false;
            if (stopping) return true;
            return false;
        };

        if (t->control_flag) {
            if (wait()) break;
            clist->clear();
            if (wait()) break;
            /** let the window manager initiate multi windows if necessary **/
            if (wait()) break;
            clist->merge(*appCL);
            //if (clist->getNumCreated() > 0) cout << "VRWindow::update " << name << " " << clist->getNumCreated() << " " << clist->getNumChanged() << endl;
            if (wait()) break;
            render(true);
            if (wait()) break;
        }

        osgSleep(1);
    } while(t && t->control_flag);
    cout << "VRWindow::update done" << endl;
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

void VRWindow::save(xmlpp::Element* node) {
    node->set_attribute("active", toString(active).c_str());
    node->set_attribute("type", toString(type).c_str());
    node->set_attribute("width", toString(width).c_str());
    node->set_attribute("height", toString(height).c_str());
    node->set_attribute("name", getName().c_str());
    if (mouse) node->set_attribute("mouse", mouse->getName().c_str());
    else if (multitouch) node->set_attribute("mouse", multitouch->getName().c_str());
    else node->set_attribute("mouse", "None");
    if (keyboard) node->set_attribute("keyboard", keyboard->getName().c_str());
    else node->set_attribute("keyboard", "None");

    xmlpp::Element* vn;
    for (auto wv : views) {
        if (auto v = wv.lock()) {
            vn = node->add_child("View");
            v->save(vn);
        }
    }
}

void VRWindow::load(xmlpp::Element* node) {
    active = toValue<bool>( node->get_attribute("active")->get_value() );
    type = toInt( node->get_attribute("type")->get_value() );
    width = toInt( node->get_attribute("width")->get_value() );
    height = toInt( node->get_attribute("height")->get_value() );
    name = node->get_attribute("name")->get_value();

    for (xmlpp::Node* n : node->get_children()) {
        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        if (el->get_name() != "View") continue;

        int i = VRSetup::getCurrent()->addView(name);
        VRViewPtr v = VRSetup::getCurrent()->getView(i);
        addView(v);
        v->load(el);
    }

    string _mouse = node->get_attribute("mouse")->get_value();
    if (_mouse != "None") {
        auto dev = VRSetup::getCurrent()->getDevice(_mouse);
        mouse = dynamic_pointer_cast<VRMouse>( dev );
        multitouch = dynamic_pointer_cast<VRMultiTouch>(dev);
        if (views.size() > 0 && mouse) if (auto v = views[0].lock()) mouse->setViewport(v);
        if (multitouch) multitouch->setWindow(ptr());
    }

    if (node->get_attribute("keyboard") != 0) {
        string _keyboard = node->get_attribute("keyboard")->get_value();
        if (_keyboard != "None") {
            keyboard = dynamic_pointer_cast<VRKeyboard>( VRSetup::getCurrent()->getDevice(_keyboard) );
        }
    }
}

OSG_END_NAMESPACE;
