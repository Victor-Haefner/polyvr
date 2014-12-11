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
#include <vte-0.0/vte/vte.h>
#include <iostream>
//#include <boost/locale.hpp>
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/windows/VRView.h"
#include "core/utils/VRInternalMonitor.h"
#include "core/scene/VRSceneLoader.h"
#include "VRGuiUtils.h"
#include "VRGuiSignals.h"
#include "VRGuiFile.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRScene.h"
#include "PolyVR.h"
#include "core/objects/VRCamera.h"

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

void VRGuiBits_on_viewoption_changed(GtkComboBox* cb, gpointer data) {
    int i = gtk_combo_box_get_active(cb);
    if (i == -1) return;

    // get all in
    VRScene* scene = VRSceneManager::getCurrent();
    VRSetup* setup = VRSetupManager::getCurrent();
    Glib::RefPtr<Gtk::ListStore> opt_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("view_options"));
    VRGuiSetup_ViewOptsColumns cols;
    Gtk::TreeModel::Row row = *getComboboxIter("combobox20");
    string opt = row.get_value(cols.option);
    bool b = row.get_value(cols.state);
    b = !b;

    // process option
    if (opt == "referentials") scene->showReferentials(b);
    if (opt == "setup") setup->showSetup(b);
    if (opt == "lights and cameras") scene->showLightsCameras(b);

    // update liststore toggle
    gtk_list_store_set (opt_list->gobj(), row.gobj(), 1, (int)b, -1);
    setCombobox("combobox20", -1);
}

void VRGuiBits_on_camera_changed(GtkComboBox* cb, gpointer data) {
    int i = gtk_combo_box_get_active(cb);
    VRScene* scene = VRSceneManager::getCurrent();
    scene->setActiveCamera(i);

    VRGuiSignals::get()->getSignal("camera_changed")->trigger();
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

    map<string, string> vars = mnr->getVariables();
    map<string, string>::iterator itr;
    for (itr = vars.begin(); itr != vars.end(); itr++) {
        Gtk::ListStore::Row row = *store->append();
        gtk_list_store_set (store->gobj(), row.gobj(), 0, itr->first.c_str(), -1);
        gtk_list_store_set (store->gobj(), row.gobj(), 1, itr->second.c_str(), -1);
    }
}

// --------------------------
// ---------Main-------------
// --------------------------

VteTerminal* terminal;

void VRGuiBits::write_to_terminal(string s) {
    for (int i=s.size(); i>=0; i--)
        if (s[i] == '\n') s.insert(i, "\r");

    vte_terminal_feed(terminal, s.c_str(), s.size());
}

void VRGuiBits::hideAbout(int i) {
    Gtk::AboutDialog* diag;
    VRGuiBuilder()->get_widget("aboutdialog1", diag);
    diag->hide();
}

bool VRGuiBits::toggleFullscreen(GdkEventKey* k) {
    if (k->keyval != 65480) return false;
    static bool fs = false;
    fs = !fs;

    Gtk::Window* win; VRGuiBuilder()->get_widget("window1", win);
    Gtk::Separator* hs1; VRGuiBuilder()->get_widget("hseparator1", hs1);
    Gtk::Table* tab; VRGuiBuilder()->get_widget("table20", tab);
    Gtk::Notebook* nb1; VRGuiBuilder()->get_widget("notebook1", nb1);
    Gtk::Box* hb1; VRGuiBuilder()->get_widget("hbox1", hb1);

    if (fs) {
        win->fullscreen();
        nb1->hide();
        hb1->hide();
        tab->hide();
        hs1->hide();
        gtk_widget_hide(term_box);
    } else {
        win->unfullscreen();
        win->show_all();
    }

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

VRGuiBits::VRGuiBits() {
    setComboboxCallback("combobox4", VRGuiBits_on_camera_changed);
    setComboboxCallback("combobox9", VRGuiBits_on_navigation_changed);

    setToolButtonCallback("toolbutton4", VRGuiBits_on_save_clicked);
    setToolButtonCallback("toolbutton3", VRGuiBits_on_quit_clicked);
    setToolButtonCallback("toolbutton17", VRGuiBits_on_about_clicked);
    setToolButtonCallback("toolbutton18", VRGuiBits_on_internal_clicked);

    setButtonCallback("button14", VRGuiBits_on_new_cancel_clicked);
    setButtonCallback("button21", VRGuiBits_on_internal_close_clicked);

    setLabel("label24", "Project: None");

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

    // VTE

    GtkWidget* vte = vte_terminal_new();
    terminal = VTE_TERMINAL (vte);

    vte_terminal_set_background_transparent(terminal, FALSE);
    vte_terminal_set_scrollback_lines(terminal, -1);
    vte_terminal_set_size(terminal, 80, 20);

    char** argv=NULL;
    g_shell_parse_argv("/bin/bash", NULL, &argv, NULL);
    vte_terminal_fork_command_full(terminal, VTE_PTY_DEFAULT, NULL, argv, NULL, GSpawnFlags(0), NULL, NULL, NULL, NULL);

    vte_terminal_set_scroll_on_keystroke(terminal, TRUE);
    gtk_widget_set_size_request(vte, -1, 100);

    GtkWidget* scrollbar = gtk_vscrollbar_new(vte_terminal_get_adjustment(terminal));
    term_box = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(term_box), vte, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(term_box), scrollbar, FALSE, FALSE, 0);

    Gtk::VPaned* paned;
    VRGuiBuilder()->get_widget("vpaned1", paned);
    gtk_paned_pack2(GTK_PANED (paned->gobj()), term_box, FALSE, FALSE);

    vte_terminal_get_emulation(VTE_TERMINAL (vte));

    gtk_widget_show (term_box);
    gtk_widget_show (vte);

    //int pos = paned->property_max_position () - 100;
    //paned->set_position(pos);

    VRFunction<int>* fkt = new VRFunction<int>( "IntMonitor_guiUpdate", VRGuiBits_on_internal_update );
    VRSceneManager::get()->addUpdateFkt(fkt);

    // view options
    setComboboxCallback("combobox20", VRGuiBits_on_viewoption_changed);
    Glib::RefPtr<Gtk::ListStore> opt_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("view_options"));
    opt_list->clear();
    Gtk::ListStore::Row row;
    row = *opt_list->append();
    gtk_list_store_set (opt_list->gobj(), row.gobj(), 0, "referentials", -1);
    gtk_list_store_set (opt_list->gobj(), row.gobj(), 1, 0, -1);
    row = *opt_list->append();
    gtk_list_store_set (opt_list->gobj(), row.gobj(), 0, "setup", -1);
    gtk_list_store_set (opt_list->gobj(), row.gobj(), 1, 0, -1);
    row = *opt_list->append();
    gtk_list_store_set (opt_list->gobj(), row.gobj(), 0, "lights and cameras", -1);
    gtk_list_store_set (opt_list->gobj(), row.gobj(), 1, 0, -1);
}

void VRGuiBits::update() {
    VRScene* scene = VRSceneManager::getCurrent();
    setLabel("label24", "Project: None");
    if (scene == 0) return;

    fillStringListstore("cameras", scene->getCameraNames());
    fillStringListstore("nav_presets", scene->getNavigationNames());

    setCombobox("combobox4", scene->getActiveCameraIndex());
    setCombobox("combobox9", getListStorePos( "nav_presets", scene->getActiveNavigation() ) );

    // update setup and project label
    cout << " now running: " << scene->getName() << endl;
    setLabel("label24", "Project: " + scene->getName());
}

OSG_END_NAMESPACE;
