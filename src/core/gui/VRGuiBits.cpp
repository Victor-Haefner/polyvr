#include "VRGuiBits.h"
#include "VRGuiConsole.h"

#include "core/scene/VRSceneManager.h"
#include "core/setup/windows/VRGtkWindow.h"
#include "core/setup/windows/VRView.h"
#include "core/utils/VRInternalMonitor.h"
#include "core/utils/VRVisualLayer.h"
#include "core/utils/VROptions.h"
#include "core/utils/VRUtilsFwd.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRSceneLoader.h"
#include "core/scripting/VRScript.h"
#include "VRGuiUtils.h"
#include "VRGuiBuilder.h"
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

#include <iostream>
#include <gtk/gtk.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

// --------------------------
// ---------SIGNALS----------
// --------------------------

void VRGuiBits::on_view_option_toggle(VRVisualLayer* l, GtkToggleToolButton* tb) {
    bool b = gtk_toggle_tool_button_get_active(tb);
    l->setVisibility( b );
}

void VRGuiBits::on_camera_changed() {
    if (update_ward) return;
    string name = getComboboxText("combobox4");
    auto scene = VRScene::getCurrent();
    scene->setActiveCamera(name);
    VRGuiManager::broadcast("camera_changed");
}

void VRGuiBits::on_navigation_changed() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    string name = getComboboxText("combobox9");
    scene->setActiveNavigation(name);
    setCombobox("combobox5", getListStorePos("nav_presets", name));
    setTooltip("combobox9", scene->getNavigationTip(name) );
}

void VRGuiBits::on_save_clicked() {
    saveScene();
}

void VRGuiBits::on_quit_clicked() {
    PolyVR::shutdown();
}

void VRGuiBits::on_web_export_clicked() {
    string D = VRSceneManager::get()->getOriginalWorkdir();
    string project = VRScene::getCurrent()->getFile();
    string projectName = VRScene::getCurrent()->getFileName();

    string folder = D+"/ressources/webBuild";
    if (!exists(folder+"/.git"))
        systemCall("git clone https://github.com/Victor-Haefner/polyvr-webport.git \"" + folder + "\"");

    systemCall("git -C \"" + folder + "\" pull");
    systemCall("cp -f \"" + folder + "/polyvr.wasm\" ./");
    systemCall("cp -f \"" + folder + "/polyvr.js\" ./");
    systemCall("cp -f \"" + folder + "/storage.js\" ./");
    systemCall("cp -f \"" + folder + "/scanDir.php\" ./");
    systemCall("cp -f \"" + folder + "/Mono.ttf\" ./");
    systemCall("cp -f \"" + folder + "/Browser.xml\" ./");

    // generate html file
    systemCall("cp -f \"" + folder + "/polyvr.html\" ./"+projectName+".html");
    fileReplaceStrings("./"+projectName+".html", "PROJECT.pvr", project);

    // TODO: table widget to present preloaded files to user
    auto preloadFile = [&](const string& path) {
        string newStr = "preloadFile('" + path + "');\n\t\t\t//INCLUDE_PRELOAD_HOOK";
        fileReplaceStrings("./"+projectName+".html", "//INCLUDE_PRELOAD_HOOK", newStr);
    };

    map<string, bool> preloadedFiles;

    // check scripts for paths to ressources
    for (auto script : VRScene::getCurrent()->getScripts()) {
        if (script.second->getType() != "Python") continue;
        string core = script.second->getCore();

        // search for strings
        vector<size_t> positions;
        bool inString = false;
        char strQuote = '"';
        for (size_t i=0; i<core.size(); i++) {
            char c = core[i];
            if (!inString) {
                if (c == '\'') { positions.push_back(i); inString = true; strQuote = c; }
                if (c == '"') { positions.push_back(i); inString = true; strQuote = c; }
            } else {
                if (c == strQuote) {
                    positions.push_back(i); inString = false;
                }
            }
        }

        for (int i=0; i<positions.size(); i+=2) {
            size_t i1 = positions[i]+1;
            size_t i2 = positions[i+1];
            string str = core.substr(i1,i2-i1);
            if (str.size() > 50) continue;
            if (exists(str) && isFile(str)) {
                if (preloadedFiles.count(str)) continue;
                preloadedFiles[str] = true;
                cout << "preloadFile " << str << endl;
                preloadFile(str);
                string osbCache = getFolderName(str)+"/."+getFileName(str)+".osb";
                if (exists(osbCache)) preloadFile(osbCache); // check for binary chaches
                else cout << " did not find osb cache '" << osbCache << "'" << endl;
            }
        }
    }

    //systemCall("gedit ./"+projectName+".html");
    if (askUser("Web build files copied to project directory", "Start in browser (google-chrome)?"))
        systemCall("google-chrome --new-window http://localhost:5500/"+projectName+".html");
}

void VRGuiBits::on_about_clicked() {
    GtkDialog* diag = (GtkDialog*)VRGuiBuilder::get()->get_widget("aboutdialog1");
    gtk_dialog_run(diag);
}

void VRGuiBits::on_fullscreen_clicked() {
    toggleWidgets();
    notifyUser("To Exit Fullscreen..", "Press both, F11 and F12");
    toggleFullscreen();
}

void VRGuiBits::on_internal_clicked() {
    auto diag = VRGuiBuilder::get()->get_widget("dialog2");
    gtk_widget_show_all(diag);
}

void VRGuiBits::on_internal_close_clicked() {
    GtkWidget* diag = VRGuiBuilder::get()->get_widget("dialog2");
    gtk_widget_hide(diag);
}

void VRGuiBits_on_internal_update() {
    GtkWidget* diag = VRGuiBuilder::get()->get_widget("dialog2");
    if (!gtk_widget_is_visible(diag)) return;

    VRInternalMonitor* mnr = VRInternalMonitor::get();
    GtkListStore* store = (GtkListStore*)VRGuiBuilder::get()->get_object("liststore4");
    gtk_list_store_clear(store);

    for (auto var : mnr->getVariables()) {
        GtkTreeIter itr;
        gtk_list_store_append(store, &itr);
        gtk_list_store_set(store, &itr, 0, var.first.c_str(), -1);
        gtk_list_store_set(store, &itr, 1, var.second.c_str(), -1);
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
    GtkWidget* diag = VRGuiBuilder::get()->get_widget("aboutdialog1");
    gtk_widget_hide(diag);
}

void VRGuiBits::toggleWidgets() {
    static bool fs = false;
    fs = !fs;

    GtkWidget* win = VRGuiBuilder::get()->get_widget("window1");
    GtkWidget* hs1 = VRGuiBuilder::get()->get_widget("hseparator1");
    GtkWidget* tab = VRGuiBuilder::get()->get_widget("table20");
    GtkWidget* nb1 = VRGuiBuilder::get()->get_widget("notebook1");
    GtkWidget* hb1 = VRGuiBuilder::get()->get_widget("hbox1");
    GtkWidget* hb2 = VRGuiBuilder::get()->get_widget("hbox15");
    GtkWidget* hp1 = VRGuiBuilder::get()->get_widget("hpaned1");

    if (fs) {
        //gtk_widget_hide(nb1);
        gtk_paned_set_position(GTK_PANED(hp1), 0);
        gtk_paned_set_wide_handle(GTK_PANED(hp1), false);
        gtk_widget_hide(hb1);
        gtk_widget_hide(hb2);
        gtk_widget_hide(tab);
        gtk_widget_hide(hs1);
    }
    else {
        gtk_paned_set_position(GTK_PANED(hp1), 410);
        gtk_paned_set_wide_handle(GTK_PANED(hp1), true);
        gtk_widget_show_all(win);
    }
}

void VRGuiBits::toggleFullscreen() {
    static bool fs = false;
    fs = !fs;
    GtkWindow* win = (GtkWindow*)VRGuiBuilder::get()->get_widget("window1");
    if (fs) gtk_window_fullscreen(win);
    else gtk_window_unfullscreen(win);
}

void VRGuiBits::toggleStereo() {
    auto win = VRSetup::getCurrent()->getEditorWindow();
    for (auto v : win->getViews()) {
        if (v == 0) continue;
        bool b = v->isStereo();
        v->setStereo(!b);
    }
}

bool VRGuiBits::pressFKey(GdkEventKey* k) {
    if (k->keyval == 65479) toggleStereo();
    if (k->keyval == 65480) toggleFullscreen();
    if (k->keyval == 65481) toggleWidgets();

    GtkWindow* win = (GtkWindow*)VRGuiBuilder::get()->get_widget("window1");
    gtk_window_propagate_key_event(GTK_WINDOW(win), k);
    return true;
}

void VRGuiBits::toggleDock() { // TODO: redesign
    GtkToggleButton* tbut = (GtkToggleButton*)VRGuiBuilder::get()->get_widget("togglebutton1");
    bool a = gtk_toggle_button_get_active(tbut);

    static GtkWindow* win = 0;
    GtkVBox* box = (GtkVBox*)VRGuiBuilder::get()->get_widget("vbox5");
    GtkVPaned* pan = (GtkVPaned*)VRGuiBuilder::get()->get_widget("vpaned1");

    if (a) {
        win = (GtkWindow*)gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(win, "PolyVR 3D View");
        gtk_window_set_default_size(win, 400, 400);
        gtk_widget_reparent((GtkWidget*)box, (GtkWidget*)win);
        gtk_widget_show_all((GtkWidget*)win);
    } else if (win) {
        gtk_widget_reparent((GtkWidget*)box, (GtkWidget*)pan);
        gtk_widget_show_all((GtkWidget*)pan);
        delete win;
    }

    //TODO: reset changelist to redraw everything!
}

void VRGuiBits::toggleVerbose(string s) {
    if (s == "network") VRLog::setTag("net", getToggleToolButtonState("network_verbose"));
}

VRGuiBits::VRGuiBits() {
    bool standalone = VROptions::get()->getOption<bool>("standalone");
    if (standalone) {
        GtkWidget* win = VRGuiBuilder::get()->get_widget("window1");
        connect_signal<bool, GdkEventKey*>(win, bind(&VRGuiBits::pressFKey, this, placeholders::_1), "key_press_event");
        connect_signal<void>(win, bind(&VRGuiBits::on_quit_clicked, this), "destroy");
        return;
    }

    setComboboxCallback("combobox4", bind(&VRGuiBits::on_camera_changed, this));
    setComboboxCallback("combobox9", bind(&VRGuiBits::on_navigation_changed, this));

    setToolButtonCallback("toolbutton4", bind(&VRGuiBits::on_save_clicked, this));
    setToolButtonCallback("toolbutton50", bind(&VRGuiBits::on_web_export_clicked, this));
    setToolButtonCallback("toolbutton3", bind(&VRGuiBits::on_quit_clicked, this));
    setToolButtonCallback("toolbutton17", bind(&VRGuiBits::on_about_clicked, this));
    setToolButtonCallback("toolbutton18", bind(&VRGuiBits::on_internal_clicked, this));
    setToolButtonCallback("toolbutton26", bind(&VRGuiBits::on_fullscreen_clicked, this));

    setButtonCallback("button21", bind(&VRGuiBits::on_internal_close_clicked, this));

    setToolButtonCallback("togglebutton1", bind(&VRGuiBits::toggleDock, this) );
    setToolButtonCallback("network_verbose", bind(&VRGuiBits::toggleVerbose, this, "network" ) );

    setLabel("label24", "Project: None");

    disableDestroyDiag("aboutdialog1");
    disableDestroyDiag("dialog2");

    // recorder
    recorder_visual_layer = VRVisualLayer::getLayer("Recorder", "recorder.png", 1);
    recToggleCb = VRFunction<bool>::create("recorder toggle", bind(&VRGuiRecWidget::setVisible, &recorder, placeholders::_1));
    recorder_visual_layer->setCallback( recToggleCb );

    // About Dialog
    GtkAboutDialog* diag = (GtkAboutDialog*)VRGuiBuilder::get()->get_widget("aboutdialog1");
    function<void(int)> sig = bind(&VRGuiBits::hideAbout, this, placeholders::_1);
    connect_signal((GtkWidget*)diag, sig, "response");
    ifstream f("ressources/gui/authors");

    vector<string> authors;
    for (string line; getline(f, line); ) authors.push_back(line);
    f.close();

    const gchar** auths = (const gchar**)malloc((authors.size()+1)*sizeof(gchar*));
    for (int i = 0; i<authors.size(); i++) auths[i] = authors[i].c_str();
    auths[authors.size()] = NULL;
    gtk_about_dialog_set_authors(diag, auths);
    free(auths);

    // window fullscreen
    GtkWidget* win = VRGuiBuilder::get()->get_widget("window1");
    connect_signal<bool,GdkEventKey*>(win, bind(&VRGuiBits::pressFKey, this, placeholders::_1), "key_press_event");
    connect_signal<void>(win, bind(&VRGuiBits::on_quit_clicked, this), "destroy");

    // TERMINAL
    terminal = (GtkNotebook*)gtk_notebook_new();
    auto addTermTab = [&](string name) {
        auto c = VRConsoleWidgetPtr( new VRConsoleWidget() );
        GtkLabel* label = (GtkLabel*)gtk_label_new(name.c_str());
        gtk_notebook_append_page(terminal, (GtkWidget*)c->getWindow(), (GtkWidget*)label);
        c->setLabel( label );
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

    GtkWidget* box = VRGuiBuilder::get()->get_widget("hbox15");
    gtk_box_pack_start((GtkBox*)box, (GtkWidget*)terminal, true, true, 0);
    gtk_widget_show_all(box);
    connect_signal<void, GtkWidget*, guint>(terminal, bind(&VRGuiBits::on_console_switch, this, placeholders::_1, placeholders::_2), "switch_page");

    updatePtr = VRUpdateCb::create( "IntMonitor_guiUpdate", VRGuiBits_on_internal_update );
    VRSceneManager::get()->addUpdateFkt(updatePtr);

    updateVisualLayer();
}

void VRGuiBits::on_console_switch(GtkWidget* page, guint page_num) {
    auto p = gtk_notebook_get_nth_page(terminal, page_num);
    string name = gtk_notebook_get_tab_label_text(terminal, p);
    openConsole->setOpen(false);
    openConsole = consoles[name];
    openConsole->setOpen(true);
}

void VRGuiBits::updateVisualLayer() {
    auto bar = VRGuiBuilder::get()->get_widget("toolbar6");
    clearContainer(bar);

    for (auto l : VRVisualLayer::getLayers()) {
        auto lay = VRVisualLayer::getLayer(l).get();
        auto ttb = gtk_toggle_tool_button_new();
        auto icon = gtk_image_new();

        gtk_tool_item_set_tooltip_markup(ttb, l.c_str());
        gtk_toolbar_insert((GtkToolbar*)bar, (GtkToolItem*)ttb, -1);

        connect_signal<void>(ttb, bind(&VRGuiBits::on_view_option_toggle, this, lay, (GtkToggleToolButton*)ttb), "toggled");

        string icon_path = VRSceneManager::get()->getOriginalWorkdir() + "/ressources/gui/" + lay->getIconName();
        gtk_image_set_from_file((GtkImage*)icon, icon_path.c_str());
        auto pbuf = gtk_image_get_pixbuf((GtkImage*)icon);
        if (pbuf) {
            pbuf = gdk_pixbuf_scale_simple((GdkPixbuf*)pbuf, 24, 24, GDK_INTERP_BILINEAR);
            gtk_image_set_from_pixbuf((GtkImage*)icon, pbuf);
            gtk_tool_button_set_icon_widget((GtkToolButton*)ttb, icon);
        }
    }

    gtk_widget_show_all(bar);
}

void VRGuiBits::update() { // scene changed
	cout << "VRGuiBits::update" << endl;
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
    cout << " VRGuiBits::update done" << endl;
}

void VRGuiBits::wipeConsoles() {
    for (auto c : consoles) c.second->clear();
}

OSG_END_NAMESPACE;
