#include "VRGuiBits.h"

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
#include <gtkmm/textview.h>
#include <gtkmm/scrolledwindow.h>
//#include <vte-0.0/vte/vte.h>
#include <iostream>
//#include <boost/locale.hpp>
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/windows/VRView.h"
#include "core/utils/VRInternalMonitor.h"
#include "core/utils/VRVisualLayer.h"
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

void VRGuiBits_on_camera_changed(GtkComboBox* cb, gpointer data) {
    char* cam = gtk_combo_box_get_active_text(cb);
    if (cam == 0) return;
    VRScene* scene = VRSceneManager::getCurrent();
    string name = string(cam);
    scene->setActiveCamera(name);

    VRGuiSignals::get()->getSignal("camera_changed")->trigger<VRDevice>();
}

void VRGuiBits_on_navigation_changed(GtkComboBox* cb, gpointer data) {
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;

    char* c = gtk_combo_box_get_active_text(cb);
    if (c == 0) return;
    string name = string(c);
    scene->setActiveNavigation(name);
}

void VRGuiBits_on_new_cancel_clicked(GtkButton* cb, gpointer data) {
    Gtk::Window* dialog;
    VRGuiBuilder()->get_widget("NewProject", dialog);
    dialog->hide();
}

void VRGuiBits_on_save_clicked(GtkButton* cb, gpointer data) {
    saveScene();
}

void VRGuiBits_on_quit_clicked(GtkButton* cb, gpointer data) {
    exitPolyVR();
}

void VRGuiBits_on_about_clicked(GtkButton* cb, gpointer data) {
    Gtk::AboutDialog* diag;
    VRGuiBuilder()->get_widget("aboutdialog1", diag);
    diag->run();
}

void VRGuiBits_on_internal_clicked(GtkButton* cb, gpointer data) {
    Gtk::Dialog* diag;
    VRGuiBuilder()->get_widget("dialog2", diag);
    cout << "\nPRINT NAME MAP" << flush;
    VRName::printNameDict();
    diag->run();
}

void VRGuiBits_on_internal_close_clicked(GtkButton* cb, gpointer data) {
    Gtk::Dialog* diag;
    VRGuiBuilder()->get_widget("dialog2", diag);
    diag->hide();
}

void VRGuiBits_on_internal_update(int i) {
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

Glib::RefPtr<Gtk::TextBuffer> terminal;

void VRGuiBits::write_to_terminal(string s) {
    boost::mutex::scoped_lock lock(msg_mutex);
    msg_queue.push(s);
}

void VRGuiBits::clear_terminal() {
    boost::mutex::scoped_lock lock(msg_mutex);
    std::queue<string>().swap(msg_queue);
    terminal->set_text("");
}

void VRGuiBits::update_terminal() {
    boost::mutex::scoped_lock lock(msg_mutex);
    while(!msg_queue.empty()) {
        terminal->insert(terminal->end(), msg_queue.front());
		msg_queue.pop();
    }
}

void VRGuiBits::on_terminal_changed() {
    if (swin == 0) return;
    auto a = swin->get_vadjustment();
    a->set_value(a->get_upper() - a->get_page_size());
}

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
    VRView* v = VRSetupManager::getCurrent()->getView(0);
    if (v == 0) return false;

    bool b = v->isStereo();
    v->setStereo(!b);
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
    setComboboxCallback("combobox4", VRGuiBits_on_camera_changed);
    setComboboxCallback("combobox9", VRGuiBits_on_navigation_changed);

    setToolButtonCallback("toolbutton4", VRGuiBits_on_save_clicked);
    setToolButtonCallback("toolbutton3", VRGuiBits_on_quit_clicked);
    setToolButtonCallback("toolbutton17", VRGuiBits_on_about_clicked);
    setToolButtonCallback("toolbutton18", VRGuiBits_on_internal_clicked);

    setToolButtonCallback("toolbutton24", sigc::mem_fun(*this, &VRGuiBits::clear_terminal));
    setToolButtonCallback("toolbutton25", sigc::mem_fun(*this, &VRGuiBits::on_terminal_changed));

    setButtonCallback("button14", VRGuiBits_on_new_cancel_clicked);
    setButtonCallback("button21", VRGuiBits_on_internal_close_clicked);

    setToolButtonCallback("togglebutton1", sigc::mem_fun(*this, &VRGuiBits::toggleDock) );
    setToolButtonCallback("network_verbose", sigc::bind<string>( sigc::mem_fun(*this, &VRGuiBits::toggleVerbose), "network" ) );

    setLabel("label24", "Project: None");

    // recorder
    recorder = new VRRecorder();
    recorder->setView(0);
    recorder_visual_layer = new VRVisualLayer("Recorder", "recorder.png");
    recorder_visual_layer->setCallback( recorder->getToggleCallback() );

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
    terminal = Gtk::TextBuffer::create();
    Gtk::TextView* term_view = Gtk::manage(new Gtk::TextView(terminal));
    Pango::FontDescription fdesc;
    fdesc.set_family("monospace");
    fdesc.set_size(10 * PANGO_SCALE);
    term_view->modify_font(fdesc);
    swin = Gtk::manage(new Gtk::ScrolledWindow());
    swin->add(*term_view);
    swin->set_size_request(-1,70);
    term_box = (GtkWidget*)swin->gobj();

    Gtk::HBox* box;
    VRGuiBuilder()->get_widget("hbox15", box);
    box->pack_start(*swin, true, true);
    box->show_all();

    swin->get_vadjustment()->signal_changed().connect( sigc::mem_fun(*this, &VRGuiBits::on_terminal_changed) );

    VRFunction<int>* fkt = new VRFunction<int>( "IntMonitor_guiUpdate", VRGuiBits_on_internal_update );
    VRSceneManager::get()->addUpdateFkt(fkt);

    updateVisualLayer();
}

void VRGuiBits::updateVisualLayer() {
    Gtk::Toolbar* bar;
    VRGuiBuilder()->get_widget("toolbar6", bar);
    for (auto c : bar->get_children()) bar->remove(*c);

    for (auto l : VRVisualLayer::getLayers()) {
        VRVisualLayer* ly = VRVisualLayer::getLayer(l);
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
    VRScene* scene = VRSceneManager::getCurrent();
    setLabel("label24", "Project: None");
    if (scene == 0) return;

    fillStringListstore("cameras", scene->getCameraNames());
    fillStringListstore("nav_presets", scene->getNavigationNames());

    setCombobox("combobox4", scene->getActiveCameraIndex());
    setCombobox("combobox9", getListStorePos( "nav_presets", scene->getActiveNavigation() ) );

    // update setup && project label
    cout << " now running: " << scene->getName() << endl;
    setLabel("label24", "Project: " + scene->getName());

    updateVisualLayer();
}

OSG_END_NAMESPACE;
