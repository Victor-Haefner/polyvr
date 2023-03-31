#include <OpenSG/OSGRenderAction.h>

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

OSG_BEGIN_NAMESPACE;
using namespace std;

// --------------------------
// ---------SIGNALS----------
// --------------------------

void VRGuiBits::on_view_option_toggle(VRVisualLayer* l, bool b) {
    if (l) l->setVisibility( b );
}

void VRGuiBits::on_camera_changed() {
    if (update_ward) return;
    string name = getComboboxText("combobox4");
    auto scene = VRScene::getCurrent();
    scene->setActiveCamera(name);
    VRGuiManager::broadcast("camera_changed");
}

void VRGuiBits::on_navigation_clicked(bool b) {
    setWidgetVisibility("navOverlay", b);
}

void VRGuiBits::on_navigation_toggled(VRNavPresetWeakPtr np, bool b) {
    auto npreset = np.lock();
    //npreset->setActive(v);
    auto scene = VRScene::getCurrent();
    if (scene) scene->setNavigationState(npreset->getName(), b);
}

void VRGuiBits::on_save_clicked() {
    saveScene();
}

void VRGuiBits::on_quit_clicked() {
    PolyVR::shutdown();
}

static string wasmServerSend =
"\nfunction send(m) {\n"
"    window.parent.postMessage(m, window.origin);\n"
"}\n";

static string wasmServerReceive =
"window.addEventListener('message', (event) => {\n"
"    handle(event.data);\n"
"}, false);\n";

string wrapTimeout(string code, string delay) {
    return "setTimeout(function(){ "+code+" }, "+delay+");";
}

void VRGuiBits::updateWebPortRessources() {
    bool withXR = getCheckButtonState("wed_opt_xr");

    int startOpt = getRadioButtonState("wed_opt_start1");
    startOpt +=  2*getRadioButtonState("wed_opt_start2");
    startOpt +=  3*getRadioButtonState("wed_opt_start3");

    string D = VRSceneManager::get()->getOriginalWorkdir();
    string project = VRScene::getCurrent()->getFile();
    string projectName = VRScene::getCurrent()->getFileName();

    string folder = D+"/ressources/webBuild";
    if (!exists(folder+"/.git"))
        systemCall("git clone https://github.com/Victor-Haefner/polyvr-webport.git \"" + folder + "\"");

    // copy websites
    //if (!exists("./websites")) makedir("./websites");
    for (auto script : VRScene::getCurrent()->getScripts()) {
        if (script.second->getType() != "HTML") continue;
        string core = script.second->getCore();

        string onOpen = "";
        auto itr = core.find("websocket.onopen"); // get the code executed on ws open
        if (itr != string::npos) {
            auto itr2 = core.find("{", itr);
            if (itr2 != string::npos) {
                auto itr3 = core.find("}", itr2);
                if (itr3 != string::npos) {
                    onOpen = core.substr(itr2+1, itr3-itr2-1);
                    cout << " on open action: " << onOpen << endl;
                }
            }
        }

        itr = core.find("function send("); // delete that line, then insert wasmServerSend
        if (itr != string::npos) {
            auto itr2 = core.find("\n", itr);
            if (itr2 != string::npos) {
                core.erase(itr, itr2-itr);
                core.insert(itr, wasmServerSend);
            }
        }

        itr = core.find("var websocket"); // prepend wasmServerReceive
        if (itr != string::npos) core.insert(itr, wasmServerReceive + wrapTimeout(onOpen, "1000") + "\n\t/*");

        itr = core.find("websocket.onclose"); // close the comment to disable the websocket
        if (itr != string::npos) {
            auto itr2 = core.find("\n", itr);
            if (itr2 != string::npos) core.insert(itr2, "*/");
        }

        ofstream out(script.first+".html");
        out << core;
        out.close();
    }

    systemCall("git -C \"" + folder + "\" pull");
    systemCall("cp -f \"" + folder + "/polyvr.wasm\" ./");
    systemCall("cp -f \"" + folder + "/polyvr.js\" ./");
    systemCall("cp -f \"" + folder + "/editor.js\" ./");
    systemCall("cp -f \"" + folder + "/editor.css\" ./");
    systemCall("cp -f \"" + folder + "/editor._html\" ./");
    systemCall("cp -f \"" + folder + "/webxr.js\" ./");
    systemCall("cp -f \"" + folder + "/webxr.css\" ./");
    systemCall("cp -f \"" + folder + "/storage.js\" ./");
    systemCall("cp -f \"" + folder + "/proxy.php\" ./");
    systemCall("cp -f \"" + folder + "/scanDir.php\" ./");
    systemCall("cp -f \"" + folder + "/Mono.ttf\" ./");
    systemCall("cp -f \"" + folder + "/Browser.xml\" ./");
    systemCall("cp -f \"" + folder + "/proj.db\" ./");
    systemCall("cp -f \"" + folder + "/polyvr.html\" ./"+projectName+".html");
    fileReplaceStrings("./"+projectName+".html", "PROJECT.pvr", project);

    // TODO: table widget to present preloaded files to user
    auto preloadFile = [&](const string& path) {
        string newStr = "preloadFile('" + path + "');\n\t\t\t//INCLUDE_PRELOAD_HOOK";
        fileReplaceStrings("./"+projectName+".html", "//INCLUDE_PRELOAD_HOOK", newStr);
        fileReplaceStrings("./"+projectName+"_editor.html", "//INCLUDE_PRELOAD_HOOK", newStr);
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
            if (str.size() > 250) continue;
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

    vector<string> options;

    if (withXR) options.push_back("webXR");
    if (startOpt == 3) options.push_back("editor");

    string optionstr = "";
    for (int i=0; i<options.size(); i++) {
        if (i > 0) optionstr += "&";
        else optionstr = "?";
        optionstr += options[i];
    }

    if (startOpt > 1)
        systemCall("google-chrome --new-window \"http://localhost:5500/"+projectName+".html"+optionstr+"\"");
}

void VRGuiBits::on_web_export_clicked() {
    uiSignal("toolbar_export");
}

void VRGuiBits::on_web_cancel() {
    uiSignal("dialog_export_cancel");
}

void VRGuiBits::on_web_start() {
    uiSignal("dialog_export_start");
    updateWebPortRessources();
}

void VRGuiBits::on_about_clicked() {
    uiSignal("toolbar_about");
}

void VRGuiBits::on_fullscreen_clicked() {
    toggleWidgets();
    notifyUser("To Exit Fullscreen..", "Press both, F11 and F12");
    toggleFullscreen();
}

void VRGuiBits::on_internal_clicked() {
    uiSignal("toolbar_profiler");
}

void VRGuiBits::on_internal_close_clicked() {
    uiSignal("dialog_profiler_close");
}

void VRGuiBits_on_internal_update() {
    uiSignal("dialog_profiler_update");

    /*GtkWidget* diag = VRGuiBuilder::get()->get_widget("dialog2");
    if (!gtk_widget_is_visible(diag)) return;

    VRInternalMonitor* mnr = VRInternalMonitor::get();
    GtkListStore* store = (GtkListStore*)VRGuiBuilder::get()->get_object("liststore4");
    gtk_list_store_clear(store);

    for (auto var : mnr->getVariables()) {
        GtkTreeIter itr;
        gtk_list_store_append(store, &itr);
        gtk_list_store_set(store, &itr, 0, var.first.c_str(), -1);
        gtk_list_store_set(store, &itr, 1, var.second.c_str(), -1);
    }*/
}

// --------------------------
// ---------Main-------------
// --------------------------

void VRGuiBits::update_terminals() {
    for (auto c : consoles) c.second->update();
}

VRConsoleWidgetPtr VRGuiBits::getConsole(string t) { return consoles[t]; }

void VRGuiBits::hideAbout(int i) {
    uiSignal("dialog_about_close");
}

void VRGuiBits::toggleWidgets() {
    static bool fs = false;
    fs = !fs;
    uiSignal("set_editor_widgets_visibility", {{"visibility",toString(fs)}});

    /*GtkWidget* win = VRGuiBuilder::get()->get_widget("window1");
    GtkWidget* hs1 = VRGuiBuilder::get()->get_widget("hseparator1");
    GtkWidget* tab = VRGuiBuilder::get()->get_widget("table20");
    GtkWidget* hb1 = VRGuiBuilder::get()->get_widget("hbox1");
    GtkWidget* hp1 = VRGuiBuilder::get()->get_widget("hpaned1");
    GtkWidget* hp2 = VRGuiBuilder::get()->get_widget("vpaned1");
    GtkWidget* hbl = VRGuiBuilder::get()->get_widget("hbox1_layout");
    GtkWidget* hp1C1 = gtk_paned_get_child1(GTK_PANED(hp1));
    GtkWidget* hp2C1 = gtk_paned_get_child2(GTK_PANED(hp2));

    if (fs) {
        //gtk_paned_set_position(GTK_PANED(hp1), 0);
        gtk_paned_set_wide_handle(GTK_PANED(hp1), false);
        gtk_paned_set_wide_handle(GTK_PANED(hp2), false);
        gtk_widget_hide(hb1);
        gtk_widget_hide(hbl);
        gtk_widget_hide(tab);
        gtk_widget_hide(hs1);
        gtk_widget_hide(hp1C1);
        gtk_widget_hide(hp2C1);
    }
    else {
        //gtk_paned_set_position(GTK_PANED(hp1), 410);
        gtk_paned_set_wide_handle(GTK_PANED(hp1), true);
        gtk_paned_set_wide_handle(GTK_PANED(hp2), true);
        gtk_widget_show_all(win);
    }*/
}

void VRGuiBits::toggleFullscreen() {
    static bool fs = false;
    fs = !fs;
    uiSignal("set_editor_fullscreen", {{"fullscreen",toString(fs)}});
    /*GtkWindow* win = (GtkWindow*)VRGuiBuilder::get()->get_widget("window1");
    if (fs) gtk_window_fullscreen(win);
    else gtk_window_unfullscreen(win);*/
}

void VRGuiBits::toggleStereo() {
    auto win = VRSetup::getCurrent()->getEditorWindow();
    for (auto v : win->getViews()) {
        if (v == 0) continue;
        bool b = v->isStereo();
        v->setStereo(!b);
    }
}

bool VRGuiBits::pressFKey(int k) {
    if (k == 65479) toggleStereo();
    if (k == 65480) toggleFullscreen();
    if (k == 65481) toggleWidgets();
    return true;
}

void VRGuiBits::toggleDock(bool b) {
    uiSignal("ui_toggle_dock", {{"docking",toString(b)}});

    /*GtkToggleButton* tbut = (GtkToggleButton*)VRGuiBuilder::get()->get_widget("togglebutton1");
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
    }*/

    //TODO: reset changelist to redraw everything!
}

void VRGuiBits::toggleVerbose(string s) {
    if (s == "network") VRLog::setTag("net", getToggleToolButtonState("network_verbose"));
}

VRGuiBits::VRGuiBits() {
    /*bool standalone = VROptions::get()->getOption<bool>("standalone");
    if (standalone) {
        GtkWidget* win = VRGuiBuilder::get()->get_widget("window1");
        connect_signal<bool, GdkEventKey*>(win, bind(&VRGuiBits::pressFKey, this, placeholders::_1), "key_press_event");
        connect_signal<void>(win, bind(&VRGuiBits::on_quit_clicked, this), "destroy");
        return;
    }*/

    /*setComboboxCallback("combobox4", bind(&VRGuiBits::on_camera_changed, this));
    setToggleButtonCallback("navButton", bind(&VRGuiBits::on_navigation_clicked, this));

    setToolButtonCallback("toolbutton4", bind(&VRGuiBits::on_save_clicked, this));
    setToolButtonCallback("toolbutton50", bind(&VRGuiBits::on_web_export_clicked, this));
    setToolButtonCallback("toolbutton3", bind(&VRGuiBits::on_quit_clicked, this));
    setToolButtonCallback("toolbutton17", bind(&VRGuiBits::on_about_clicked, this));
    setToolButtonCallback("toolbutton18", bind(&VRGuiBits::on_internal_clicked, this));
    setToolButtonCallback("toolbutton26", bind(&VRGuiBits::on_fullscreen_clicked, this));

    setButtonCallback("button21", bind(&VRGuiBits::on_internal_close_clicked, this));

    setButtonCallback("wed_cancel", bind(&VRGuiBits::on_wed_cancel, this));
    setButtonCallback("wed_start", bind(&VRGuiBits::on_wed_start, this));

    setToolButtonCallback("togglebutton1", bind(&VRGuiBits::toggleDock, this) );
    setToolButtonCallback("network_verbose", bind(&VRGuiBits::toggleVerbose, this, "network" ) );

    setLabel("label24", "Project: None");
    //setLabel("lblversion", getVersionString()); // in about dialog

    disableDestroyDiag("aboutdialog1");
    disableDestroyDiag("dialog2");*/

    // recorder
    recorder_visual_layer = VRVisualLayer::getLayer("Recorder", "recorder.png", 1);
    recToggleCb = VRFunction<bool>::create("recorder toggle", bind(&VRGuiRecWidget::setVisible, &recorder, placeholders::_1));
    recorder_visual_layer->setCallback( recToggleCb );

    // About Dialog
    /*GtkAboutDialog* diag = (GtkAboutDialog*)VRGuiBuilder::get()->get_widget("aboutdialog1");
    function<void(int)> sig = bind(&VRGuiBits::hideAbout, this, placeholders::_1);
    connect_signal((GtkWidget*)diag, sig, "response");*/
    ifstream f("ressources/gui/authors");
    vector<string> authors;
    for (string line; getline(f, line); ) authors.push_back(line);
    f.close();
    /*const gchar** auths = (const gchar**)malloc((authors.size()+1)*sizeof(gchar*));
    for (int i = 0; i<authors.size(); i++) auths[i] = authors[i].c_str();
    auths[authors.size()] = NULL;
    gtk_about_dialog_set_authors(diag, auths);
    free(auths);

    gtk_about_dialog_set_version(diag, getVersionString());

    // window fullscreen
    GtkWidget* win = VRGuiBuilder::get()->get_widget("window1");
    connect_signal<bool,GdkEventKey*>(win, bind(&VRGuiBits::pressFKey, this, placeholders::_1), "key_press_event");
    connect_signal<void>(win, bind(&VRGuiBits::on_quit_clicked, this), "destroy");*/

    // TERMINAL
    //terminal = (GtkNotebook*)gtk_notebook_new();
    auto addTermTab = [&](string name) {
        auto c = VRConsoleWidgetPtr( new VRConsoleWidget() );
        c->setLabel( name );
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
    auto colTab = addTermTab("Collaboration");

    colTab->addStyle( "red", "#ff3311", "#ffffff", false, false, false, true );
    colTab->addStyle( "green", "#00cc11", "#ffffff", false, false, false, true );

    openConsole = consoles["Console"];
    openConsole->setOpen(true);

    /*GtkWidget* box = VRGuiBuilder::get()->get_widget("hbox15");
    gtk_box_pack_start((GtkBox*)box, (GtkWidget*)terminal, true, true, 0);
    gtk_widget_show_all(box);
    connect_signal<void, GtkWidget*, guint>(terminal, bind(&VRGuiBits::on_console_switch, this, placeholders::_1, placeholders::_2), "switch_page");*/

    updatePtr = VRUpdateCb::create( "IntMonitor_guiUpdate", VRGuiBits_on_internal_update );
    VRSceneManager::get()->addUpdateFkt(updatePtr);

    updateVisualLayer();
}

void VRGuiBits::on_console_switch(string name) {
    //auto p = gtk_notebook_get_nth_page(terminal, page_num);
    //string name = gtk_notebook_get_tab_label_text(terminal, p);
    openConsole->setOpen(false);
    openConsole = consoles[name];
    openConsole->setOpen(true);
}

void VRGuiBits::updateVisualLayer() {
    //auto bar = VRGuiBuilder::get()->get_widget("toolbar6");
    //clearContainer(bar);

    for (auto l : VRVisualLayer::getLayers()) {
        auto lay = VRVisualLayer::getLayer(l).get();
        //GtkToolItem* ttb = 0;

        string icon_path = VRSceneManager::get()->getOriginalWorkdir() + "/ressources/gui/" + lay->getIconName();
        if (exists(icon_path)) {
            /*ttb = gtk_toggle_tool_button_new();
            auto icon = gtk_image_new();
            gtk_image_set_from_file((GtkImage*)icon, icon_path.c_str());
            auto pbuf = gtk_image_get_pixbuf((GtkImage*)icon);
            if (pbuf) {
                pbuf = gdk_pixbuf_scale_simple((GdkPixbuf*)pbuf, 24, 24, GDK_INTERP_BILINEAR);
                gtk_image_set_from_pixbuf((GtkImage*)icon, pbuf);
                gtk_tool_button_set_icon_widget((GtkToolButton*)ttb, icon);
            }*/
        } else { // try stock image
            //ttb = gtk_toggle_tool_button_new_from_stock(lay->getIconName().c_str());
        }

        /*gtk_tool_item_set_tooltip_markup(ttb, l.c_str());
        gtk_toolbar_insert((GtkToolbar*)bar, (GtkToolItem*)ttb, -1);

        connect_signal<void>(ttb, bind(&VRGuiBits::on_view_option_toggle, this, lay, (GtkToggleToolButton*)ttb), "toggled");*/
    }

    //gtk_widget_show_all(bar);
}

bool VRGuiBits::update() { // scene changed
	cout << "VRGuiBits::update" << endl;
    update_ward = true;
    auto scene = VRScene::getCurrent();
    setLabel("label24", "Project: None");
    if (scene == 0) return true;

    fillStringListstore("cameras", scene->getCameraNames());
    fillStringListstore("nav_presets", scene->getNavigationNames());

    setCombobox("combobox4", scene->getActiveCameraIndex());
    //setCombobox("combobox9", getListStorePos( "nav_presets", scene->getActiveNavigation() ) );

    // update setup && project label
    setLabel("label24", "Project: " + scene->getName());

    updateVisualLayer();
    update_ward = false;
    cout << " VRGuiBits::update done" << endl;

    /*auto navOverlay = VRGuiBuilder::get()->get_widget("navOverlay");
    clearContainer(navOverlay);*/
    for (auto nav : scene->getNavigations()) {
        /*auto row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        auto cb = gtk_check_button_new_with_label(nav.second->getBaseName().c_str());
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), nav.second->isActive());
        setTooltip(cb, scene->getNavigationTip(nav.first) );
        setCheckButtonCallback(cb, bind(&VRGuiBits::on_navigation_toggled, this, nav.second, cb));

        gtk_box_pack_start(GTK_BOX(row), cb, false, true, 0);
        gtk_box_pack_start(GTK_BOX(navOverlay), row, false, true, 0);
        gtk_widget_show_all(row);*/
    }

    return true;
}

void VRGuiBits::wipeConsoles() {
    for (auto c : consoles) c.second->clear();
}

OSG_END_NAMESPACE;
