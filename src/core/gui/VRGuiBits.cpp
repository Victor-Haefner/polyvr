#include "VRGuiBits.h"
#include "VRGuiConsole.h"

#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/paned.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/frame.h>
#include <gtkmm/notebook.h>
#include <gtkmm/separator.h>
#include <gtkmm/table.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/builder.h>
#include <gtkmm/scrolledwindow.h>
#include <iostream>

#include "core/scene/VRSceneManager.h"
#include "core/setup/windows/VRGtkWindow.h"
#include "core/setup/windows/VRView.h"
#include "core/utils/VRInternalMonitor.h"
#include "core/utils/VRVisualLayer.h"
#include "core/utils/VRUtilsFwd.h"
#include "core/scene/VRSceneLoader.h"
#include "VRGuiUtils.h"
#include "VRGuiSignals.h"
#include "VRGuiFile.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRScene.h"
#include "PolyVR.h"
#include "core/objects/VRCamera.h"
#include "core/tools/VRRecorder.h"
#include "core/utils/VRLogger.h"
#include "core/setup/devices/VRSignal.h"
#include "VRGuiManager.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiSetup_ViewOptsColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiSetup_ViewOptsColumns() { add(option); add(state); }

        Gtk::TreeModelColumn<Glib::ustring> option;
        Gtk::TreeModelColumn<gint> state;
};


// --------------------------
// ---------SIGNALS----------
// --------------------------

void VRGuiBits::on_view_option_toggle(VRVisualLayer* l, Gtk::ToggleToolButton* tb) {
    l->setVisibility( tb->get_active() );
}

void VRGuiBits::on_camera_changed() {
    if (update_ward) return;
    string name = getComboboxText("combobox4");
    auto scene = VRScene::getCurrent();
    scene->setActiveCamera(name);
    VRGuiSignals::get()->getSignal("camera_changed")->triggerPtr<VRDevice>();
}

void VRGuiBits::on_navigation_changed() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    string name = getComboboxText("combobox9");
    scene->setActiveNavigation(name);
    setCombobox("combobox5", getListStorePos("nav_presets", name));
    setTooltip("combobox9", scene->getNavigationTip(name) );
}

void VRGuiBits::on_new_cancel_clicked() {
    Gtk::Window* dialog;
    VRGuiBuilder()->get_widget("NewProject", dialog);
    dialog->hide();
}

void VRGuiBits::on_save_clicked() {
    saveScene();
}

void VRGuiBits::on_quit_clicked() {
    PolyVR::shutdown();
}

void VRGuiBits::on_about_clicked() {
    Gtk::AboutDialog* diag;
    VRGuiBuilder()->get_widget("aboutdialog1", diag);
    diag->run();
}

void VRGuiBits::on_internal_clicked() {
    Gtk::Dialog* diag;
    VRGuiBuilder()->get_widget("dialog2", diag);
    cout << "\nPRINT NAME MAP" << flush;
    VRName::printInternals();
    diag->run();
}

void VRGuiBits::on_internal_close_clicked() {
    Gtk::Dialog* diag;
    VRGuiBuilder()->get_widget("dialog2", diag);
    diag->hide();
}

void VRGuiBits_on_internal_update() {
    VRInternalMonitor* mnr = VRInternalMonitor::get();
    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("liststore4"));
    store->clear();

    for (auto var : mnr->getVariables()) {
        Gtk::ListStore::Row row = *store->append();
        gtk_list_store_set (store->gobj(), row.gobj(), 0, var.first.c_str(), -1);
        gtk_list_store_set (store->gobj(), row.gobj(), 1, var.second.c_str(), -1);
    }
}

// --------------------------
// ---------Main-------------
// --------------------------

void VRGuiBits::update_terminals() {
    for (auto c : consoles) c.second->update();
}

VRConsoleWidgetPtr VRGuiBits::getConsole(string t) { return consoles[t]; }

void VRGuiBits::hideAbout(int i) {
    Gtk::AboutDialog* diag;
    VRGuiBuilder()->get_widget("aboutdialog1", diag);
    diag->hide();
}

bool VRGuiBits::toggleWidgets(GdkEventKey* k) {
    if (k->keyval != 65481) return false;
    static bool fs = false;
    fs = !fs;

    Gtk::Window* win; VRGuiBuilder()->get_widget("window1", win);
    Gtk::Separator* hs1; VRGuiBuilder()->get_widget("hseparator1", hs1);
    Gtk::Table* tab; VRGuiBuilder()->get_widget("table20", tab);
    Gtk::Notebook* nb1; VRGuiBuilder()->get_widget("notebook1", nb1);
    Gtk::Box* hb1; VRGuiBuilder()->get_widget("hbox1", hb1);
    Gtk::Box* hb2; VRGuiBuilder()->get_widget("hbox15", hb2);

    if (fs) {
        nb1->hide();
        hb1->hide();
        hb2->hide();
        tab->hide();
        hs1->hide();
    } else win->show_all();
    return true;
}

bool VRGuiBits::toggleFullscreen(GdkEventKey* k) {
    if (k->keyval != 65480) return false;
    static bool fs = false;
    fs = !fs;

    Gtk::Window* win; VRGuiBuilder()->get_widget("window1", win);
    if (fs) win->fullscreen();
    else win->unfullscreen();
    return true;
}

bool VRGuiBits::toggleStereo(GdkEventKey* k) {
    if (k->keyval != 65479) return false;

    auto win = VRSetup::getCurrent()->getEditorWindow();
    for (auto v : win->getViews()) {
        if (v == 0) continue;
        bool b = v->isStereo();
        v->setStereo(!b);
    }

    return true;
}

void VRGuiBits::toggleDock() {
    Gtk::ToggleToolButton* tbut;
    VRGuiBuilder()->get_widget("togglebutton1", tbut);
    bool a = tbut->get_active();

    static Gtk::Window* win = 0;
    Gtk::VBox* box;
    Gtk::VPaned* pan;
    VRGuiBuilder()->get_widget("vbox5", box);
    VRGuiBuilder()->get_widget("vpaned1", pan);

    if(a) {
        win = new Gtk::Window();
        win->set_title("PolyVR 3D View");
        win->set_default_size(400, 400);
        box->reparent(*win);
        win->show_all();
    } else if(win) {
        box->reparent(*pan);
        pan->show_all();
        delete win;
    }

    //TODO: reset changelist to redraw everything!
}

void VRGuiBits::toggleVerbose(string s) {
    if (s == "network") VRLog::setTag("net", getToggleButtonState("network_verbose"));
}

VRGuiBits::VRGuiBits() {
    setComboboxCallback("combobox4", sigc::mem_fun(*this, &VRGuiBits::on_camera_changed));
    setComboboxCallback("combobox9", sigc::mem_fun(*this, &VRGuiBits::on_navigation_changed));

    setToolButtonCallback("toolbutton4", sigc::mem_fun(*this, &VRGuiBits::on_save_clicked));
    setToolButtonCallback("toolbutton3", sigc::mem_fun(*this, &VRGuiBits::on_quit_clicked));
    setToolButtonCallback("toolbutton17", sigc::mem_fun(*this, &VRGuiBits::on_about_clicked));
    setToolButtonCallback("toolbutton18", sigc::mem_fun(*this, &VRGuiBits::on_internal_clicked));

    setButtonCallback("button14", sigc::mem_fun(*this, &VRGuiBits::on_new_cancel_clicked));
    setButtonCallback("button21", sigc::mem_fun(*this, &VRGuiBits::on_internal_close_clicked));

    setToolButtonCallback("togglebutton1", sigc::mem_fun(*this, &VRGuiBits::toggleDock) );
    setToolButtonCallback("network_verbose", sigc::bind<string>( sigc::mem_fun(*this, &VRGuiBits::toggleVerbose), "network" ) );

    setLabel("label24", "Project: None");

    // recorder
    recorder_visual_layer = VRVisualLayer::getLayer("Recorder", "recorder.png", 1);
    recToggleCb = VRFunction<bool>::create("recorder toggle", boost::bind(&VRGuiRecWidget::setVisible, &recorder, _1));
    recorder_visual_layer->setCallback( recToggleCb );

    // About Dialog
    Gtk::AboutDialog* diag;
    VRGuiBuilder()->get_widget("aboutdialog1", diag);
    diag->signal_response().connect( sigc::mem_fun(*this, &VRGuiBits::hideAbout) );
    ifstream f("ressources/gui/authors");
    vector<string> authors;
    for (string line; getline(f, line); ) authors.push_back(line);
    f.close();
    diag->set_authors(authors);

    // window fullscreen
    Gtk::Window* win;
    VRGuiBuilder()->get_widget("window1", win);
    win->signal_key_press_event().connect( sigc::mem_fun(*this, &VRGuiBits::toggleStereo) );
    win->signal_key_press_event().connect( sigc::mem_fun(*this, &VRGuiBits::toggleFullscreen) );
    win->signal_key_press_event().connect( sigc::mem_fun(*this, &VRGuiBits::toggleWidgets) );

    // TERMINAL
    terminal = Gtk::manage( new Gtk::Notebook() );
    auto addTermTab = [&](string name) {
        auto c = VRConsoleWidgetPtr( new VRConsoleWidget() );
        terminal->append_page(*c->getWindow(), name);
        c->setLabel( (Gtk::Label*)terminal->get_tab_label(*c->getWindow()) );
        consoles[name] = c;
        return c;
    };

    addTermTab("Console");
    auto err1Tab = addTermTab("Errors");
    auto err2Tab = addTermTab("Syntax");
    err1Tab->configColor("#e03000");
    err2Tab->configColor("#e03000");
    addTermTab("Search results");
    addTermTab("Reasoning");
    addTermTab("Tracking");
    openConsole = consoles["Console"];
    openConsole->setOpen(true);

    Gtk::HBox* box;
    VRGuiBuilder()->get_widget("hbox15", box);
    box->pack_start(*terminal, true, true);
    box->show_all();
    terminal->signal_switch_page().connect( sigc::mem_fun(*this, &VRGuiBits::on_console_switch) );

    updatePtr = VRUpdateCb::create( "IntMonitor_guiUpdate", VRGuiBits_on_internal_update );
    VRSceneManager::get()->addUpdateFkt(updatePtr);

    updateVisualLayer();
}

void VRGuiBits::on_console_switch(GtkNotebookPage* page, guint page_num) {
    auto p = terminal->get_nth_page(page_num);
    string name = terminal->get_tab_label_text(*p);
    openConsole->setOpen(false);
    openConsole = consoles[name];
    openConsole->setOpen(true);
}

void VRGuiBits::updateVisualLayer() {
    Gtk::Toolbar* bar;
    VRGuiBuilder()->get_widget("toolbar6", bar);
    for (auto c : bar->get_children()) bar->remove(*c);

    for (auto l : VRVisualLayer::getLayers()) {
        auto ly = VRVisualLayer::getLayer(l).get();
        Gtk::ToggleToolButton* tb = Gtk::manage( new Gtk::ToggleToolButton() );
        Gtk::Image* icon = Gtk::manage( new Gtk::Image() );

        tb->set_tooltip_text(l);

        sigc::slot<void> slot = sigc::bind<VRVisualLayer*, Gtk::ToggleToolButton*>( sigc::mem_fun(*this, &VRGuiBits::on_view_option_toggle), ly, tb);
        bar->append(*tb, slot);

        string icon_path = VRSceneManager::get()->getOriginalWorkdir() + "/ressources/gui/" + ly->getIconName();
        icon->set(icon_path);
        Glib::RefPtr<Gdk::Pixbuf> pbuf = icon->get_pixbuf();
        if (pbuf) {
            pbuf = pbuf->scale_simple(24, 24, Gdk::INTERP_BILINEAR);
            icon->set(pbuf);
            tb->set_icon_widget(*icon);
        }
    }

    bar->show_all();
}

void VRGuiBits::update() { // scene changed
    update_ward = true;
    auto scene = VRScene::getCurrent();
    setLabel("label24", "Project: None");
    if (scene == 0) return;

    fillStringListstore("cameras", scene->getCameraNames());
    fillStringListstore("nav_presets", scene->getNavigationNames());

    setCombobox("combobox4", scene->getActiveCameraIndex());
    setCombobox("combobox9", getListStorePos( "nav_presets", scene->getActiveNavigation() ) );

    // update setup && project label
    setLabel("label24", "Project: " + scene->getName());

    updateVisualLayer();
    update_ward = false;
}

void VRGuiBits::wipeConsoles() {
    for (auto c : consoles) c.second->clear();
}

OSG_END_NAMESPACE;
