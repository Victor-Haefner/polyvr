#include "VRWindow.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRThreadManager.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

unsigned int VRWindow::active_window_count = 0;

VRWindow::VRWindow() {
    active_window_count++;
    string n = getName();
    auto f = new VRFunction<VRThread*>("VRWindow", boost::bind(&VRWindow::update, this, _1) );
    thread_id = VRSceneManager::get()->initThread(f,n,true,0);
}

VRWindow::~VRWindow() {
    VRSceneManager::get()->stopThread(thread_id);
    _win = NULL;
}

WindowRecPtr VRWindow::getOSGWindow() { return _win; }

void VRWindow::addView(VRView* view) {
    views.push_back(view);
    view->setWindow(_win);
}

void VRWindow::remView(VRView* view) {
    if (mouse) mouse->setViewport(0);
    views.erase(std::remove(views.begin(), views.end(), view), views.end());
}

void VRWindow::setAction(RenderActionRefPtr ract) { this->ract = ract; }
vector<VRView*> VRWindow::getViews() { return views; }
bool VRWindow::hasType(int i) { return (i == type); }
void VRWindow::resize(int w, int h) { _win->resize(w,h); }

void VRWindow::render() { _win->render(ract); }

void VRWindow::update(VRThread* t) {
    do {
        BarrierRefPtr barrier = Barrier::get("PVR_rendering", true);
        barrier->enter(active_window_count+1);

        Thread::getCurrentChangeList()->merge(*t->appThread->getChangeList());

        render();

        Thread::getCurrentChangeList()->clear();
        barrier->enter(active_window_count+1);

        osgSleep(1);
    } while(t->control_flag);
}

bool VRWindow::isActive() { return active; }
void VRWindow::setActive(bool b) { active = b; }

bool VRWindow::hasContent() { return content; }
void VRWindow::setContent(bool b) { content = b; }

void VRWindow::setMouse(VRMouse* m) { mouse = m; }
VRMouse* VRWindow::getMouse() { return mouse; }

void VRWindow::setKeyboard(VRKeyboard* k) { keyboard = k; }
VRKeyboard* VRWindow::getKeyboard() { return keyboard; }

void VRWindow::save(xmlpp::Element* node) {
    node->set_attribute("active", toString(active).c_str());
    node->set_attribute("type", toString(type).c_str());
    node->set_attribute("width", toString(width).c_str());
    node->set_attribute("height", toString(height).c_str());
    node->set_attribute("name", getName().c_str());
    if (mouse) node->set_attribute("mouse", mouse->getName().c_str());
    else node->set_attribute("mouse", "None");
    if (keyboard) node->set_attribute("keyboard", keyboard->getName().c_str());
    else node->set_attribute("keyboard", "None");

    xmlpp::Element* vn;
    for (uint i=0; i<views.size(); i++) {
        vn = node->add_child("View");
        views[i]->save(vn);
    }
}

void VRWindow::load(xmlpp::Element* node) {
    active = toBool( node->get_attribute("active")->get_value() );
    type = toInt( node->get_attribute("type")->get_value() );
    width = toInt( node->get_attribute("width")->get_value() );
    height = toInt( node->get_attribute("height")->get_value() );
    name = node->get_attribute("name")->get_value();

    xmlpp::Node::NodeList nl = node->get_children();
    xmlpp::Node::NodeList::iterator itr;
    for (itr = nl.begin(); itr != nl.end(); itr++) {
        xmlpp::Node* n = *itr;

        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        if (el->get_name() != "View") continue;

        int i = VRSetupManager::getCurrent()->addView(name);
        VRView* v = VRSetupManager::getCurrent()->getView(i);
        addView(v);
        v->load(el);
    }

    string _mouse = node->get_attribute("mouse")->get_value();
    if (_mouse != "None") {
        mouse = (VRMouse*)VRSetupManager::getCurrent()->getDevice(_mouse);
        if (views.size() > 0 and mouse) mouse->setViewport(views[0]);
    }

    if (node->get_attribute("keyboard") != 0) {
        string _keyboard = node->get_attribute("keyboard")->get_value();
        if (_keyboard != "None") {
            keyboard = (VRKeyboard*)VRSetupManager::getCurrent()->getDevice(_keyboard);
        }
    }
}

OSG_END_NAMESPACE;
