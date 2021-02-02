#include <gtk/gtk.h>
#include "VRGuiScripts.h"
#include "VRGuiUtils.h"
#include "VRGuiBuilder.h"
#include "VRGuiFile.h"
#include "VRGuiBits.h"
#include "VRGuiManager.h"
#include "VRGuiConsole.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRScene.h"
#include "core/scripting/VRScript.h"
#include "core/utils/toString.h"
#include "core/utils/VRTimer.h"
#include "core/utils/xml.h"
#include "wrapper/VRGuiTreeView.h"

#include <iostream>

OSG_BEGIN_NAMESPACE;
using namespace std;

// --------------------------
// ---------Callbacks--------
// --------------------------

VRScriptPtr VRGuiScripts::getSelectedScript() {
    VRGuiTreeView tree_view("treeview5");
    if (!tree_view.hasSelection()) return 0;

    // get selected script
    string name = tree_view.getSelectedStringValue(0);
    auto scene = VRScene::getCurrent();
    if (scene == 0) return 0;
    VRScriptPtr script = scene->getScript(name);
    return script;
}

void VRGuiScripts::setGroupListRow(GtkTreeIter* itr, group& g) {
    auto store = (GtkTreeStore*)VRGuiBuilder::get()->get_object("script_tree");
    gtk_tree_store_set (store, itr,
                        0, g.name.c_str(),
                        1, "#FFFFFF",
                        2, "#666666",
                        3, "",
                        4, "#666666",
                        5, "#FFFFFF",
                        6, "",
                        7, "",
                        8, g.ID,
                        -1);
}

void VRGuiScripts::setScriptListRow(GtkTreeIter* itr, VRScriptPtr script, bool onlyTime) {
    if (script == 0) return;
    string color = "#000000";
    string background = "#FFFFFF";
    string tfg = "#000000";
    string tbg = "#FFFFFF";

    if (script->getPersistency() == 0) { // imported script
        color = "#0000FF";
    }

    int trig_lvl = 0;
    for (auto trig : script->getTriggers()) {
        if (trig->trigger == "on_scene_load") trig_lvl |= 1;
        if (trig->trigger == "on_scene_close") trig_lvl |= 2;
        if (trig->trigger == "on_scene_import") trig_lvl |= 1;
        if (trig->trigger == "on_timeout") trig_lvl |= 4;
        if (trig->trigger == "on_device") trig_lvl |= 8;
        if (trig->trigger == "on_socket") trig_lvl |= 16;
        if (trig->trigger == "on_device_drag") trig_lvl |= 32;
        if (trig->trigger == "on_device_drop") trig_lvl |= 64;
    }
    if (script->getType() == "HTML") trig_lvl |= 128;
    if (script->getType() == "GLSL") trig_lvl |= 256;

    if (trig_lvl >= 1) tbg = "#AAFF88";
    if (trig_lvl >= 4) tbg = "#FF8866";
    if (trig_lvl >= 8) tbg = "#FFBB33";
    if (trig_lvl >= 16) tbg = "#3388FF";
    if (trig_lvl >= 32) tbg = "#FFCCAA";
    if (trig_lvl >= 64) tbg = "#FFCC88";
    if (trig_lvl >= 128) tbg = "#AACCFF";
    if (trig_lvl >= 256) tbg = "#CCAAFF";

    string time = " ";
    float exec_time = script->getExecutionTime();
    if (exec_time >= 60*1000) time = toString( exec_time*0.001/60 ) + " min";
    else if (exec_time >= 1000) time = toString( exec_time*0.001 ) + " s";
    else if (exec_time >= 0) time = toString( exec_time ) + " ms";

    auto getUserFocus = []() {
        auto win1 = (GtkWindow*)VRGuiBuilder::get()->get_widget("window1");
        GtkWidget* wdg = gtk_window_get_focus(win1);
        if (!wdg) return "";
        auto wn = gtk_widget_get_name(wdg);
        return wn?wn:"";
    };

    string name = getUserFocus();
    bool user_focus = false;
    if(!user_focus) user_focus = ("GtkTreeView" == name);
    if(!user_focus) user_focus = ("GtkEntry" == name); // TODO: be more specific
    if(onlyTime && (user_focus || !doPerf)) return;

    int Nf = script->getSearch().N;
    string icon, Nfound;
    if (Nf) {
        icon = "gtk-find";
        Nfound = toString(Nf);
    }

    auto store = (GtkTreeStore*)VRGuiBuilder::get()->get_object("script_tree");
    if (onlyTime) gtk_tree_store_set (store, itr,
                        3, time.c_str(),
                        4, tfg.c_str(),
                        5, tbg.c_str(),
                        -1);
    else gtk_tree_store_set (store, itr,
                        0, script->getName().c_str(),
                        1, color.c_str(),
                        2, background.c_str(),
                        3, time.c_str(),
                        4, tfg.c_str(),
                        5, tbg.c_str(),
                        6, icon.c_str(),
                        7, Nfound.c_str(),
                        8, -1,
                        -1);
}

void VRGuiScripts::on_new_clicked() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->newScript("Script", "\timport VR");
    updateList();
}

VRGuiScripts::group::group() { static int i = 0; ID = i; i++; }

void VRGuiScripts::on_addSep_clicked() {
    group g;
    groups[g.ID] = g;
    updateList();
}

void VRGuiScripts::on_save_clicked() {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    string core = editor->getCore(script->getHeadSize());
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->updateScript(script->getName(), core);

    setWidgetSensitivity("toolbutton7", false);

    saveScene();
}

void VRGuiScripts::on_import_clicked() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    gtk_list_store_clear(import_liststore1);
    gtk_list_store_clear(import_liststore2);
    GtkTreeIter row;
    for (auto script : scene->getScripts()) {
        gtk_list_store_append(import_liststore2, &row);
        gtk_list_store_set (import_liststore2, &row, 0, script.first.c_str(), -1);
    }

    VRGuiFile::clearFilter();
    VRGuiFile::addFilter("Project", 2, "*.xml", "*.pvr");
    VRGuiFile::addFilter("All", 1, "*");
    VRGuiFile::setWidget(scriptImportWidget, true, true);
    VRGuiFile::setCallbacks( bind(&VRGuiScripts::on_diag_import, this), function<void()>(), bind(&VRGuiScripts::on_diag_import_select, this));
    VRGuiFile::open("Import", GTK_FILE_CHOOSER_ACTION_OPEN, "Import script");
}

void VRGuiScripts::on_diag_import_select() {
    gtk_list_store_clear(import_liststore1);
    import_scripts.clear();
    string path = VRGuiFile::getPath();
    if (path == "") return;

    XML xml;
    xml.read(path, false);

    XMLElementPtr scene = xml.getRoot();
    if (!scene) return;
    auto scripts = scene->getChild("Scripts");
    if (!scripts) return;

    GtkTreeIter row;
    for (auto script : scripts->getChildren()) {
        string name = script->getName();
        if (script->hasAttribute("base_name")) {
            string suffix = script->getAttribute("name_suffix");
            if (suffix == "0") suffix = "";
            name = script->getAttribute("base_name") + suffix;
        }

        VRScriptPtr s = VRScript::create(name);
        s->enable(false);
        s->load(script);
        import_scripts[name] = s;

        gtk_list_store_append(import_liststore1, &row);
        gtk_list_store_set (import_liststore1, &row, 0, name.c_str(), -1);
    }
}

void VRGuiScripts::on_diag_import() {
    VRGuiTreeView tree_view((GtkWidget*)import_treeview1);
    if (!tree_view.hasSelection()) return;

    // get selected script
    string name = tree_view.getSelectedStringValue(0);
    if (import_scripts.count(name) == 0) return;

    VRScriptPtr s = import_scripts[name];
    import_scripts.erase(name);

    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->addScript(s);
    s->enable(true);
    updateList();
}

void VRGuiScripts::on_diag_import_select_1() {} // TODO
void VRGuiScripts::on_diag_import_select_2() {}

void VRGuiScripts::on_exec_clicked() {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    //VRTimer t; t.start();
    on_save_clicked();
    //cout << " VRGuiScripts::on_exec_clicked t1 " << t.stop() << endl; t.reset();

    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    scene->triggerScript(script->getName());
    //cout << " VRGuiScripts::on_exec_clicked t2 " << t.stop() << endl; t.reset();
    //VRGuiSignals::get()->getSignal("scene_modified")->triggerPtr<VRDevice>(); // realy needed? can take a lot of time!
    //cout << " VRGuiScripts::on_exec_clicked t3 " << t.stop() << endl; t.reset();
}

void VRGuiScripts::on_perf_toggled() {
    doPerf = getToggleToolButtonState("toggletoolbutton1");
}

void VRGuiScripts::on_pause_toggled() {
    bool b = getToggleToolButtonState("toggletoolbutton2");
    VRScene::getCurrent()->pauseScripts(b);
}

void VRGuiScripts::on_del_clicked() {
    VRGuiTreeView tree_view("treeview5");
    if (!tree_view.hasSelection()) return;

    // get selected script
    string name = tree_view.getSelectedStringValue(0);
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    VRScriptPtr script = scene->getScript(name);
    if (script == 0) return;

    string msg1 = "Delete script " + name;
    if (!askUser(msg1, "Are you sure you want to delete this script?")) return;

    scene->remScript(script->getName());
    updateList();

    setWidgetSensitivity("toolbutton9", false);
    setWidgetSensitivity("toolbutton8", false);
}

void VRGuiScripts::on_select_script() { // selected a script
    if (pages.count(selected)) {
        auto& P = pages[selected];
        editor->getCursor(P.line, P.column);
        P.line++;
        P.column++;
        //cout << "editor focus out, cursor at: " << selected << "  " << P.line << "  " << P.column << endl;
    }

    VRScriptPtr script = VRGuiScripts::getSelectedScript();
    if (script == 0) {
        setWidgetSensitivity("toolbutton8", false);
        setWidgetSensitivity("toolbutton7", false);
        setWidgetSensitivity("toolbutton9", false);
        setWidgetSensitivity("table15", false);
        selected = "";
        return;
    }

    selected = script->getName();
    trigger_cbs = false;

    // update options
    setCombobox("combobox1", getListStorePos("liststore6", script->getType()));
    auto setup = VRSetup::getCurrent();
    if (setup) fillStringListstore("liststore7", setup->getDevices("server"));
    vector<string> grps;
    grps.push_back("no group");
    for (auto g : groups) grps.push_back(g.second.name);
    fillStringListstore("liststore10", grps);
    setCombobox("combobox24", getListStorePos("liststore7", script->getServer()));
    setCombobox("combobox10", getListStorePos("liststore10", script->getGroup()));

    // update editor content
    editor->setCore(script->getScript());

    // update arguments liststore
    auto args = (GtkListStore*)VRGuiBuilder::get()->get_object("liststore2");
    gtk_list_store_clear(args);

    GtkTreeIter row;
    //if (PyErr_Occurred() != NULL) PyErr_Print();
    for (auto a : script->getArguments()) {
        gtk_list_store_append(args, &row);
        gtk_list_store_set(args, &row, 0, a->getName().c_str(), -1);
        gtk_list_store_set(args, &row, 1, a->val.c_str(), -1);
        gtk_list_store_set(args, &row, 2, script.get(), -1);
        gtk_list_store_set(args, &row, 3, a->type.c_str(), -1);
    }

    // update trigger liststore
    auto trigs = (GtkListStore*)VRGuiBuilder::get()->get_object("triggers");
    gtk_list_store_clear(trigs);
    for (auto t : script->getTriggers()) {
        string key = toString(t->key);
        if (t->dev == "keyboard" && t->key > 32 && t->key < 127) {
            char kc = t->key;
            key = kc;
        }
        gtk_list_store_append(trigs, &row);
        gtk_list_store_set(trigs, &row, 0, t->trigger.c_str(), -1);
        gtk_list_store_set(trigs, &row, 1, t->dev.c_str(), -1);
        gtk_list_store_set(trigs, &row, 2, key.c_str(), -1);
        gtk_list_store_set(trigs, &row, 3, t->state.c_str(), -1);
        gtk_list_store_set(trigs, &row, 4, t->param.c_str(), -1);
        gtk_list_store_set(trigs, &row, 5, script.get(), -1);
        gtk_list_store_set(trigs, &row, 6, t->getName().c_str(), -1);
    }

    setWidgetSensitivity("toolbutton8", true);
    setWidgetSensitivity("toolbutton7", false);
    setWidgetSensitivity("toolbutton9", true);
    setWidgetSensitivity("table15", true);

    // language
    editor->setLanguage(script->getType());

    // script trigger
    //string trigger = script->getTrigger();
    //setTextEntry("entry48", script->getTriggerParams());

    //setCombobox("combobox1", getListStorePos("ScriptTrigger", trigger));
    trigger_cbs = true;

    if (pages.count(selected)) {
        editor->grabFocus();
        pagePos P2 = pages[selected];
        editor->setCursor(P2.line, P2.column);
        //cout << "editor grab focus on " << selected << "  " << P2.line << "  " << P2.column << endl;
    }
}

// keyboard key detection
bool wait_for_key = false;
GtkTreeIter trigger_row;
void VRGuiScripts::on_select_trigger() {
    wait_for_key = false;

    VRGuiTreeView tree_view("treeview14");
    if (!tree_view.hasSelection()) return;

    tree_view.getSelection(&trigger_row);

    //pixbuf pressed?
    string col_name = tree_view.getSelectedColumnName();
    if (col_name != " ") return;

    // get key
    string device = tree_view.getSelectedStringValue(1);
    if (device != "keyboard") return;
    wait_for_key = true;
}

bool VRGuiScripts::on_any_key_event(GdkEventKey* event) {
    if (!wait_for_key) return false;
    wait_for_key = false;

    int key = event->keyval;
    VRGuiTreeView tree_view("treeview14");
    tree_view.setStringValue(&trigger_row, 2, toString(key));

    VRScript* script = (VRScript*)tree_view.getValue(&trigger_row, 5);
    string name = tree_view.getStringValue(&trigger_row, 6);
    script->changeTrigKey(name, key);
    on_select_script();
    on_save_clicked();

    return true;
}

void VRGuiScripts::on_any_event(GdkEvent* event) {
    if (event->type == GDK_KEY_PRESS) on_any_key_event((GdkEventKey*)event);
}

void VRGuiScripts::on_name_edited(const char* path, const char* new_name) {
    VRGuiTreeView tree_view("treeview5");
    if (!tree_view.hasSelection()) return;

    // set the cell with new name
    string name = tree_view.getSelectedStringValue(0);
    tree_view.setSelectedStringValue(0, new_name);

    // update data
    int type = tree_view.getSelectedIntValue(8);
    if (type == -1) {
        auto scene = VRScene::getCurrent();
        if (scene == 0) return;
        scene->changeScriptName(name, new_name);
    } else {
        cout << "VRGuiScripts::on_name_edited grp ID " << type << endl;
        groups[type].name = new_name;
        for (auto& sw : groups[type].scripts) if (auto s = sw.lock()) s->setGroup(new_name);
    }
    updateList();
    on_select_script();
}

void VRGuiScripts::on_buffer_changed() {
    setWidgetSensitivity("toolbutton7", true);

    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    // TODO
    // get in which line the changed occured
    // negate change if in line 0

    string core = editor->getCore(script->getHeadSize());
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->updateScript(script->getName(), core, false);
}

void VRGuiScripts::on_focus_out_changed(GdkEvent*) {}

shared_ptr<VRGuiEditor> VRGuiScripts::getEditor() { return editor; }

void VRGuiScripts::on_argadd_clicked() {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    script->addArgument();
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_trigadd_clicked() {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    script->addTrigger();
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_argrem_clicked() {
    VRGuiTreeView tree_view("treeview7");
    if (!tree_view.hasSelection()) return;

    // set the cell with new name
    string name = tree_view.getSelectedStringValue(0);
    auto script = (VRScript*)tree_view.getSelectedValue(2);

    script->remArgument(name);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_trigrem_clicked() {
    VRGuiTreeView tree_view("treeview14");
    if (!tree_view.hasSelection()) return;

    // set the cell with new name
    string name = tree_view.getSelectedStringValue(6);
    auto script = (VRScript*)tree_view.getSelectedValue(5);

    script->remTrigger(name);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_argname_edited(const char* path, const char* new_name) {
    VRGuiTreeView tree_view("treeview7");
    if (!tree_view.hasSelection()) return;

    // set the cell with new name
    string name = tree_view.getSelectedStringValue(0);
    tree_view.setSelectedStringValue(0, new_name);
    auto script = (VRScript*)tree_view.getSelectedValue(2);

    // update argument name
    script->changeArgName(name, new_name);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_argval_edited(const char* path, const char* new_name) {
    VRGuiTreeView tree_view("treeview7");
    if (!tree_view.hasSelection()) return;

    // set the cell with new value
    string name = tree_view.getSelectedStringValue(0);
    tree_view.setSelectedStringValue(1, new_name);
    auto script = (VRScript*)tree_view.getSelectedValue(2);

    // update argument name
    script->changeArgValue(name, new_name);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_argtype_edited(const char* new_name, GtkTreeIter* new_iter) {
    VRGuiTreeView tree_view("treeview7");
    if (!tree_view.hasSelection()) return;

    // set the cell with new type
    auto combo_list = (GtkListStore*)VRGuiBuilder::get()->get_object("arg_types");
    gchar *t;
    gtk_tree_model_get((GtkTreeModel*)combo_list, (GtkTreeIter*)new_iter, 0, &t, -1);
    string type = string(t);
    tree_view.setSelectedStringValue(3, type);

    // do something
    string name = tree_view.getSelectedStringValue(0);
    auto script = (VRScript*)tree_view.getSelectedValue(2);
    script->changeArgType(name, type);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_trigger_edited(const char* new_name, GtkTreeIter* new_iter) {
    VRGuiTreeView tree_view("treeview14");
    if (!tree_view.hasSelection()) return;

    // set the cell with new type
    auto combo_list = (GtkListStore*)VRGuiBuilder::get()->get_object("ScriptTrigger");
    gchar* t = 0;
    gtk_tree_model_get((GtkTreeModel*)combo_list, (GtkTreeIter*)new_iter, 0, &t, -1);
    if (!t) return;
    string type = string(t);
    tree_view.setSelectedStringValue(0, type);

    // do something
    string name = tree_view.getSelectedStringValue(6);
    auto script = (VRScript*)tree_view.getSelectedValue(5);
    script->changeTrigger(name, type);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_trigdev_edited(const char* new_name, GtkTreeIter* new_iter) {
    VRGuiTreeView tree_view("treeview14");
    if (!tree_view.hasSelection()) return;

    // set the cell with new type
    auto combo_list = (GtkListStore*)VRGuiBuilder::get()->get_object("ScriptTriggerDevices");
    gchar *t;
    gtk_tree_model_get((GtkTreeModel*)combo_list, (GtkTreeIter*)new_iter, 0, &t, -1);
    string type = string(t);
    tree_view.setSelectedStringValue(1, type);

    // do something
    string name = tree_view.getSelectedStringValue(6);
    auto script = (VRScript*)tree_view.getSelectedValue(5);
    script->changeTrigDev(name, type);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_trigparam_edited(const char* path, const char* new_name) {
    VRGuiTreeView tree_view("treeview14");
    if (!tree_view.hasSelection()) return;

    // set the cell with new value
    tree_view.setSelectedStringValue(4, new_name);

    // do something
    string name = tree_view.getSelectedStringValue(6);
    auto script = (VRScript*)tree_view.getSelectedValue(5);
    script->changeTrigParams(name, new_name);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_trigkey_edited(const char* path, const char* new_name) {
    VRGuiTreeView tree_view("treeview14");
    if (!tree_view.hasSelection()) return;

    // set the cell with new value
    tree_view.setSelectedStringValue(2, new_name);

    // do something
    string name = tree_view.getSelectedStringValue(6);
    auto script = (VRScript*)tree_view.getSelectedValue(5);
    script->changeTrigKey(name, toInt(new_name));
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_trigstate_edited(const char* new_name, GtkTreeIter* new_iter) {
    VRGuiTreeView tree_view("treeview14");
    if (!tree_view.hasSelection()) return;

    // set the cell with new type
    auto combo_list = (GtkListStore*)VRGuiBuilder::get()->get_object("ScriptTriggerStates");
    gchar *t;
    gtk_tree_model_get((GtkTreeModel*)combo_list, (GtkTreeIter*)new_iter, 0, &t, -1);
    string type = string(t);

    tree_view.setSelectedStringValue(3, new_name);

    // do something
    string name = tree_view.getSelectedStringValue(6);
    auto script = (VRScript*)tree_view.getSelectedValue(5);
    script->changeTrigState(name, type);
    on_select_script();
    on_save_clicked();
}

// templates dialog

void VRGuiScripts::on_template_clicked() {
    VRGuiScripts::updateTemplates();

    auto tb  = VRGuiBuilder::get()->get_object("scripttemplates");
    gtk_text_buffer_set_text((GtkTextBuffer*)tb, "", 0);

    showDialog("scriptTemplates");
    VRGuiWidget("tentry1").grabFocus();
}

void VRGuiScripts::updateTemplates() {
    auto store = (GtkTreeStore*)VRGuiBuilder::get()->get_object("templates");
    gtk_tree_store_clear(store);

    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    auto templates = scene->getTemplates();

    // apply filter
    auto sameChar = [](char c1, char c2) { return std::toupper(c1, std::locale()) == std::toupper(c2, std::locale()); };

    auto contain = [&](const string& s, const string& i) {
        return bool( search( s.begin(), s.end(), i.begin(), i.end(), sameChar ) != s.end() );
    };

    map<string, vector<string>> filtered;
    if (templ_filter != "") {
        for (auto tv : templates) {
            for (auto t : tv.second) {
                bool s = contain(t, templ_filter);
                if (s) filtered[tv.first].push_back(t);
            }
        }
    } else filtered = templates;

    auto setRow = [&](GtkTreeIter* itr, string label) {
        gtk_tree_store_set(store, itr,  0, label.c_str(), -1);
    };

    GtkTreeIter itr0, itr1;
    for (auto tv : filtered) {
        gtk_tree_store_append(store, &itr0, NULL);
        setRow(&itr0, tv.first);
        for (auto t : tv.second) {
            gtk_tree_store_append(store, &itr1, &itr0);
            setRow(&itr1, t);
        }
    }

    if (templ_filter != "") VRGuiTreeView("ttreeview1").expandAll();
}

void VRGuiScripts::on_select_templ() {
    VRGuiTreeView tree_view("ttreeview1");
    if (!tree_view.hasSelection()) return;

    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    // get selected object
    string templ  = tree_view.getSelectedStringValue(0);

    string core = scene->getTemplateCore(templ);

    auto tb = (GtkTextBuffer*)VRGuiBuilder::get()->get_object("scripttemplates");
    gtk_text_buffer_set_text(tb, core.c_str(), core.size());
}

void VRGuiScripts::on_templ_close_clicked() { hideDialog("scriptTemplates"); }

void VRGuiScripts::on_templ_import_clicked() {
    hideDialog("scriptTemplates");

    VRGuiTreeView tree_view("ttreeview1");
    if (!tree_view.hasSelection()) return;

    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    // get selected object
    string templ = tree_view.getSelectedStringValue(0);
    scene->importTemplate(templ);
    updateList();
}

void VRGuiScripts::on_templ_filter_edited() {
    templ_filter = getTextEntry("tentry1");
    updateTemplates();
}

// help dialog

void VRGuiScripts::on_help_close_clicked() {
    hideDialog("pybindings-docs");
}

void VRGuiScripts::on_help_clicked() {
    VRGuiScripts::updateDocumentation();

    auto tb  = VRGuiBuilder::get()->get_object("pydoc");
    gtk_text_buffer_set_text((GtkTextBuffer*)tb, "", 0);

    showDialog("pybindings-docs");
    VRGuiWidget("entry25").grabFocus();
}

void VRGuiScripts::updateDocumentation() {
    auto store = (GtkTreeStore*)VRGuiBuilder::get()->get_object("bindings");
    gtk_tree_store_clear(store);

    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    // apply filter
    map<string, map<string, map<string, bool> > > data;
    map<string, map<string, map<string, bool> > > filtered;

    auto sameChar = [](char c1, char c2) { return std::toupper(c1, std::locale()) == std::toupper(c2, std::locale()); };

    auto contain = [&](const string& s, const string& i) {
        return bool( search( s.begin(), s.end(), i.begin(), i.end(), sameChar ) != s.end() );
    };

    for (auto mod : scene->getPyVRModules()) {
        data[mod] = map<string, map<string, bool> >();
        for (auto typ : scene->getPyVRTypes(mod)) {
            data[mod][typ] = map<string, bool>();
            for (auto fkt : scene->getPyVRMethods(mod, typ) ) data[mod][typ][fkt] = 1;
        }
    }

    filtered = data;
    if (docs_filter != "") {
        for (auto mod : data) {
            bool showMod = contain(mod.first, docs_filter);
            if (!showMod) {
                for (auto typ : mod.second) {
                    bool showTyp = contain(typ.first, docs_filter);
                    if (!showTyp) {
                        for (auto fkt : typ.second) {
                            bool showFkt = contain(fkt.first, docs_filter);
                            if (showFkt) showTyp = true;
                            else filtered[mod.first][typ.first].erase(fkt.first);
                        }
                    }
                    if (showTyp) showMod = true;
                    else filtered[mod.first].erase(typ.first);
                }
            }
            if (!showMod) filtered.erase(mod.first);
        }
    }

    auto setRow = [&](GtkTreeIter* itr, string label, string type, string cla, string mod, string col = "#FFFFFF") {
        gtk_tree_store_set (store, itr,  0, label.c_str(),
                                                        1, type.c_str(),
                                                        2, cla.c_str(),
                                                        3, mod.c_str(),
                                                        4, col.c_str(), -1);
    };

    GtkTreeIter itr0, itr1, itr2;
    for (auto mod : filtered) {
        gtk_tree_store_append(store, &itr0, NULL);
        string modname = (mod.first == "VR") ? "VR" : "VR."+mod.first;
        setRow(&itr0, modname, "module", "", "", "#BBDDFF");
        for (auto typ : mod.second) {
            gtk_tree_store_append(store, &itr1, &itr0);
            setRow(&itr1, typ.first, "class", typ.first, mod.first);
            for (auto fkt : typ.second) {
                gtk_tree_store_append(store, &itr2, &itr1);
                setRow(&itr2, fkt.first, "method", typ.first, mod.first);
            }
        }
    }

    if (docs_filter != "") VRGuiTreeView("treeview3").expandAll();
}

void VRGuiScripts::on_select_help() {
    VRGuiTreeView tree_view("treeview3");
    if (!tree_view.hasSelection()) return;

    // get selected object
    string obj  = tree_view.getSelectedStringValue(0);
    string type = tree_view.getSelectedStringValue(1);
    string cla  = tree_view.getSelectedStringValue(2);
    string mod  = tree_view.getSelectedStringValue(3);

    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    auto tb = (GtkTextBuffer*)VRGuiBuilder::get()->get_object("pydoc");
    gtk_text_buffer_set_text(tb, "", 0);

    if (type == "class") {
        string doc = scene->getPyVRDescription(mod, obj) + "\n\n";
        for (auto method : scene->getPyVRMethods(mod, obj)) {
            string d = scene->getPyVRMethodDoc(mod, cla, method);
            doc += method + "\n\t" + d + "\n\n";
        }
        gtk_text_buffer_set_text(tb, doc.c_str(), doc.size());
    }

    if (type == "method") {
        string doc = "\n" + scene->getPyVRMethodDoc(mod, cla, obj);
        gtk_text_buffer_set_text(tb, doc.c_str(), doc.size());
    }
}

void VRGuiScripts::on_doc_filter_edited() {
    docs_filter = getTextEntry("entry25");
    updateDocumentation();
}

// script search dialog

void VRGuiScripts::on_find_clicked() {
    setToggleButton("checkbutton38", true);
    setWidgetSensitivity("entry11", false);
    string txt = editor->getSelection();
    setTextEntry("entry10", txt);
    VRGuiWidget("entry10").grabFocus();
    showDialog("find_dialog");
}

void VRGuiScripts::on_find_diag_cancel_clicked() {
    hideDialog("find_dialog");
}

VRGuiScripts::searchResult::searchResult(string s, int l, int c) : scriptName(s), line(l), column(c) {}

void VRGuiScripts::focusScript(string name, int line, int column) {
    setNotebookPage("notebook1", 2);
    setNotebookPage("notebook3", 3);

    auto store = (GtkTreeStore*)VRGuiBuilder::get()->get_object("script_tree");
    auto tree_view = (GtkTreeView*)VRGuiBuilder::get()->get_widget("treeview5");

    auto selectScript = [&](GtkTreeIter* itr) {
        string n = VRGuiTreeView((GtkWidget*)tree_view).getStringValue(itr, 0);
        if (name == n) {
            GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), itr);
            gtk_tree_view_expand_to_path(tree_view, path);
            gtk_tree_view_set_cursor(tree_view, path, NULL, false);
            return true;
        }
        return false;
    };

    auto selectScript2 = [&]() {
        GtkTreeIter child, child2;
        bool valid = gtk_tree_model_iter_children((GtkTreeModel*)store, &child, NULL);
        while (valid) {
            if (selectScript(&child)) return;

            bool valid2 = gtk_tree_model_iter_children((GtkTreeModel*)store, &child2, &child);
            while (valid2) {
                if (selectScript(&child2)) return;
                valid2 = gtk_tree_model_iter_next((GtkTreeModel*)store, &child2);
            }

            valid = gtk_tree_model_iter_next((GtkTreeModel*)store, &child);
        }
    };

    // select script in tree view
    selectScript2();

    // set focus on editor
    editor->grabFocus();
    editor->setCursor(line, column);
}

void VRGuiScripts::getLineFocus(int& line, int& column) { editor->getCursor(line, column); }

void VRGuiScripts::on_search_link_clicked(searchResult res, string s) {
    focusScript(res.scriptName, res.line, res.column);
}

void VRGuiScripts::on_find_diag_find_clicked() {
    bool sa = getCheckButtonState("checkbutton38");
    //bool rep = getCheckButtonState("checkbutton12");
    string search = getTextEntry("entry10");
    if (search == "") return;
    hideDialog("find_dialog");

    VRScriptPtr s = getSelectedScript();
    if (!sa && s == 0) return;

    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    vector<VRScriptPtr> results;
    if (!sa) results = scene->searchScript(search, s);
    else results = scene->searchScript(search);

    auto print = [&]( string m, string style = "", shared_ptr< VRFunction<string> > link = 0 ) {
        VRGuiManager::get()->getConsole( "Search results" )->write( m, style, link );
    };

    VRGuiManager::get()->getConsole( "Search results" )->addStyle( "blueLink", "#3355ff", "#ffffff", false, true, true );

    // result output
    print( "Results, line-position, for search of '" + search + "':\n");
    for (auto r : results) {
        print( r->getName()+":" );
        for (auto r2 : r->getSearch().result) {
            for (auto p : r2.second) {
                print( " " );
                stringstream out;
                out << r2.first << "-" << p;
                searchResult sRes(r->getName(), r2.first, p);
                auto fkt = VRFunction<string>::create("search_link", bind(&VRGuiScripts::on_search_link_clicked, this, sRes, _1) );
                print( out.str(), "blueLink", fkt );
            }
        }
        print( "\n" );
    }
    print( "\n" );
    updateList();
}

void VRGuiScripts::on_toggle_find_replace() {
    //setWidgetSensitivity("entry11", getCheckButtonState("checkbutton12") );
    setWidgetSensitivity("entry11", false);
    setTextEntry("entry11", "Coming soon!");
}

// config stuff

void VRGuiScripts::on_change_script_type() {
    if(!trigger_cbs) return;
    VRScriptPtr script = getSelectedScript();
    string t = getComboboxText("combobox1");
    script->setType(t);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_change_group() {
    if(!trigger_cbs) return;
    VRScriptPtr script = getSelectedScript();
    script->setGroup( getComboboxText("combobox10") );
    on_select_script();
    on_save_clicked();
    updateList();
}

void VRGuiScripts::on_change_server() {
    if(!trigger_cbs) return;
    VRScriptPtr script = getSelectedScript();
    script->setHTMLHost( getComboboxText("combobox24") );
    on_select_script();
    on_save_clicked();
}


// --------------------------
// ---------Main-------------
// --------------------------

void VRGuiScripts::on_scene_changed() {
	cout << "VRGuiScripts::on_scene_changed" << endl;
    groups.clear();
}

void VRGuiScripts::update() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    for (auto& r : scriptRows) setScriptListRow(&r.second, r.first.lock(), true);
}

void VRGuiScripts::updateList() {
	cout << "VRGuiScripts::updateList" << endl;
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    // update script list
    auto store = (GtkTreeStore*)VRGuiBuilder::get()->get_object("script_tree");
    gtk_tree_store_clear(store);

    auto oldpages = pages;
    pages.clear();

    map<string, pair<int,GtkTreeIter>> grpIter;
    auto addGroupRow = [&](group& g) {
        GtkTreeIter iter;
        gtk_tree_store_append(store, &iter, NULL);
        grpIter[g.name] = pair<int,GtkTreeIter>(g.ID,iter);
        setGroupListRow(&iter, g);
        g.scripts.clear();
    };

    for (auto& g : groups) addGroupRow(g.second); // add all groups to list

    for (auto script : scene->getScripts()) { // check for new groups
        string grp = script.second->getGroup();
        if (grp != "" && grp != "no group" && !grpIter.count(grp)) {
            group g;
            groups[g.ID] = g;
            groups[g.ID].name = grp;
            addGroupRow(groups[g.ID]);
        }
    }

    scriptRows.clear();
    for (auto script : scene->getScripts()) {
        auto s = script.second;
        auto k = s.get();
        GtkTreeIter itr;
        string grp = s->getGroup();
        if (!grpIter.count(grp)) gtk_tree_store_append(store, &itr, NULL);
        else {
            auto& g = groups[grpIter[grp].first];
            g.scripts.push_back(s);
            gtk_tree_store_append(store, &itr, &grpIter[grp].second);
        }
        scriptRows.push_back( pair<VRScriptPtr, GtkTreeIter>(s,itr) );
        setScriptListRow(&itr, s);

        string name = k->getName();
        if (oldpages.count(name)) pages[name] = oldpages[name];
        else pages[name] = pagePos();
    }
    on_select_script();
}

bool VRGuiScripts_on_editor_select(GtkWidget* widget, GdkEvent* event, VRGuiScripts* self) {
    GdkEventButton* event_btn = (GdkEventButton*)event;

    if (event->type == GDK_BUTTON_RELEASE && event_btn->button == 1) {
        auto editor = GTK_TEXT_VIEW(widget);
        auto buffer = gtk_text_view_get_buffer(editor);

        GtkTextIter A, B;
        gchar* selection = 0;
        if ( gtk_text_buffer_get_selection_bounds(buffer, &A, &B) ) {
            selection = gtk_text_buffer_get_text(buffer, &A, &B, true);
        }
        self->getEditor()->highlightStrings(selection?selection:"", "asSelected");
        return false;
    }

    if (event->type == GDK_KEY_RELEASE || event->type == GDK_BUTTON_RELEASE) { // remove highlights on any key or button
        self->getEditor()->highlightStrings("", "asSelected");
        return false;
    }
    return false;
}

bool VRGuiScripts::on_help_close_frame_clicked(GdkEvent* event) {
    auto diag = VRGuiBuilder::get()->get_widget("pybindings-docs");
    gtk_widget_hide(diag);
    return true;
}

namespace PL = std::placeholders;

VRGuiScripts::VRGuiScripts() {
    disableDestroyDiag("pybindings-docs");
    disableDestroyDiag("find_dialog");
    disableDestroyDiag("scriptTemplates");

    setToolButtonCallback("toolbutton6", bind(&VRGuiScripts::on_new_clicked, this) );
    setToolButtonCallback("toolbutton29", bind(&VRGuiScripts::on_template_clicked, this) );
    setToolButtonCallback("toolbutton7", bind(&VRGuiScripts::on_save_clicked, this) );
    setToolButtonCallback("toolbutton8", bind(&VRGuiScripts::on_exec_clicked, this) );
    setToolButtonCallback("toolbutton9", bind(&VRGuiScripts::on_del_clicked, this) );
    setToolButtonCallback("toolbutton16", bind(&VRGuiScripts::on_help_clicked, this) );
    setToolButtonCallback("toolbutton20", bind(&VRGuiScripts::on_addSep_clicked, this) );
    setToolButtonCallback("toolbutton22", bind(&VRGuiScripts::on_import_clicked, this) );
    setToolButtonCallback("toolbutton23", bind(&VRGuiScripts::on_find_clicked, this) );
    setToolButtonCallback("toggletoolbutton1", bind(&VRGuiScripts::on_perf_toggled, this) );
    setToolButtonCallback("toggletoolbutton2", bind(&VRGuiScripts::on_pause_toggled, this) );

    setButtonCallback("button12", bind(&VRGuiScripts::on_argadd_clicked, this) );
    setButtonCallback("button13", bind(&VRGuiScripts::on_argrem_clicked, this) );
    setButtonCallback("button23", bind(&VRGuiScripts::on_trigadd_clicked, this) );
    setButtonCallback("button24", bind(&VRGuiScripts::on_trigrem_clicked, this) );
    setButtonCallback("button16", bind(&VRGuiScripts::on_help_close_clicked, this) );
    setButtonCallback("tbutton1", bind(&VRGuiScripts::on_templ_close_clicked, this) );
    setButtonCallback("tbutton2", bind(&VRGuiScripts::on_templ_import_clicked, this) );
    setButtonCallback("button28", bind(&VRGuiScripts::on_find_diag_cancel_clicked, this) );
    setButtonCallback("button29", bind(&VRGuiScripts::on_find_diag_find_clicked, this) );

    setToggleButtonCallback("checkbutton12", bind(&VRGuiScripts::on_toggle_find_replace, this) );

    setComboboxCallback("combobox1", bind(&VRGuiScripts::on_change_script_type, this) );
    setComboboxCallback("combobox10", bind(&VRGuiScripts::on_change_group, this) );
    setComboboxCallback("combobox24", bind(&VRGuiScripts::on_change_server, this) );

    // trigger tree_view
    auto tree_view = VRGuiBuilder::get()->get_widget("treeview14");
    gtk_widget_add_events(tree_view, (int)GDK_KEY_PRESS_MASK);
    connect_signal<void, GdkEvent*>(tree_view, bind(&VRGuiScripts::on_any_event, this, PL::_1), "event-after");
    connect_signal<bool, GdkEventKey*>(tree_view, bind(&VRGuiScripts::on_any_key_event, this, PL::_1), "key_press_event");

    setTreeviewSelectCallback("treeview14", bind(&VRGuiScripts::on_select_trigger, this) );
    setTreeviewSelectCallback("treeview5", bind(&VRGuiScripts::on_select_script, this) );
    setTreeviewSelectCallback("treeview3", bind(&VRGuiScripts::on_select_help, this) );
    setTreeviewSelectCallback("ttreeview1", bind(&VRGuiScripts::on_select_templ, this) );

    editor = shared_ptr<VRGuiEditor>( new VRGuiEditor("scrolledwindow4") );
    editor->addKeyBinding("find", VRUpdateCb::create("findCb", bind(&VRGuiScripts::on_find_clicked, this)));
    editor->addKeyBinding("save", VRUpdateCb::create("saveCb", bind(&VRGuiScripts::on_save_clicked, this)));
    editor->addKeyBinding("exec", VRUpdateCb::create("execCb", bind(&VRGuiScripts::on_exec_clicked, this)));
    connect_signal<void>(editor->getSourceBuffer(), bind(&VRGuiScripts::on_buffer_changed, this), "changed");
    //connect_signal<void, GdkEvent*>(editor->getEditor(), bind(&VRGuiScripts::on_focus_out_changed, this, PL::_1), "focus-out-event");

    setEntryCallback("entry10", bind(&VRGuiScripts::on_find_diag_find_clicked, this), false, false);

    setCellRendererCallback("cellrenderertext13", bind(&VRGuiScripts::on_name_edited, this, PL::_1, PL::_2) );
    setCellRendererCallback("cellrenderertext2", bind(&VRGuiScripts::on_argname_edited, this, PL::_1, PL::_2) );
    setCellRendererCallback("cellrenderertext14", bind(&VRGuiScripts::on_argval_edited, this, PL::_1, PL::_2) );
    setCellRendererCallback("cellrenderertext16", bind(&VRGuiScripts::on_trigparam_edited, this, PL::_1, PL::_2) );
    setCellRendererCallback("cellrenderertext41", bind(&VRGuiScripts::on_trigkey_edited, this, PL::_1, PL::_2) );

    setCellRendererCombo("treeviewcolumn16", "arg_types", 3, bind(&VRGuiScripts::on_argtype_edited, this, PL::_1, PL::_2) );
    setCellRendererCombo("treeviewcolumn27", "ScriptTrigger", 0, bind(&VRGuiScripts::on_trigger_edited, this, PL::_1, PL::_2) );
    setCellRendererCombo("treeviewcolumn28", "ScriptTriggerDevices", 1, bind(&VRGuiScripts::on_trigdev_edited, this, PL::_1, PL::_2) );
    setCellRendererCombo("treeviewcolumn30", "ScriptTriggerStates", 3, bind(&VRGuiScripts::on_trigstate_edited, this, PL::_1, PL::_2) );

    // fill combolists
    const char *arg_types[] = {"int", "float", "str", "VRPyObjectType", "VRPyTransformType", "VRPyGeometryType", "VRPyLightType", "VRPyLodType", "VRPyDeviceType", "VRPyMouseType", "VRPyHapticType", "VRPySocketType", "VRPyLeapFrameType"};
    const char *trigger_types[] = {"none", "on_scene_load", "on_scene_close", "on_scene_import", "on_timeout", "on_device", "on_socket"};
    const char *device_types[] = {"mouse", "multitouch", "keyboard", "flystick", "haptic", "server1", "leap", "vrpn_device"}; // TODO: get from a list in devicemanager or something
    const char *trigger_states[] = {"Pressed", "Released", "Drag", "Drop", "To edge", "From edge"};
    const char *script_types[] = {"Python", "GLSL", "HTML"};
    fillStringListstore("arg_types", vector<string>(arg_types, end(arg_types)) );
    fillStringListstore("ScriptTrigger", vector<string>(trigger_types, end(trigger_types)) );
    fillStringListstore("ScriptTriggerDevices", vector<string>(device_types, end(device_types)) );
    fillStringListstore("ScriptTriggerStates", vector<string>(trigger_states, end(trigger_states)) );
    fillStringListstore("liststore6", vector<string>(script_types, end(script_types)) );

    setWidgetSensitivity("toolbutton7", false);
    setWidgetSensitivity("toolbutton8", false);
    setWidgetSensitivity("toolbutton9", false);

    // update the list each frame to update the execution time
    updatePtr = VRUpdateCb::create("scripts_gui_update",  bind(&VRGuiScripts::update, this) );
    VRSceneManager::get()->addUpdateFkt(updatePtr, 100);

    sceneChangedCb = VRFunction<VRDeviceWeakPtr>::create("GUI_sceneChanged", bind(&VRGuiScripts::on_scene_changed, this) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( sceneChangedCb );

    // init scriptImportWidget
    scriptImportWidget = (GtkTable*)gtk_table_new(0,0,true);
    GtkScrolledWindow* sw1 = (GtkScrolledWindow*)gtk_scrolled_window_new(0,0);
    GtkScrolledWindow* sw2 = (GtkScrolledWindow*)gtk_scrolled_window_new(0,0);
    import_treeview1 = (GtkTreeView*)gtk_tree_view_new();
    import_treeview2 = (GtkTreeView*)gtk_tree_view_new();

/*class VRGuiScripts_ModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiScripts_ModelColumns() { add(script); add(fg); add(bg); add(time); add(tfg); add(tbg); add(icon); add(Nfound); add(type); }
        Gtk::TreeModelColumn<Glib::ustring> script; 0
        Gtk::TreeModelColumn<Glib::ustring> fg;     1
        Gtk::TreeModelColumn<Glib::ustring> bg;     2
        Gtk::TreeModelColumn<Glib::ustring> time;   3
        Gtk::TreeModelColumn<Glib::ustring> tfg;    4
        Gtk::TreeModelColumn<Glib::ustring> tbg;    5
        Gtk::TreeModelColumn<Glib::ustring> icon;   6
        Gtk::TreeModelColumn<Glib::ustring> Nfound; 7
        Gtk::TreeModelColumn<gint> type;            8
};*/

    import_liststore1 = gtk_list_store_new(9, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    import_liststore2 = gtk_list_store_new(9, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

    GtkAttachOptions Opt = GtkAttachOptions(GTK_EXPAND|GTK_FILL);
    gtk_table_attach(scriptImportWidget, (GtkWidget*)sw1, 0,1,0,1,  Opt, Opt, 0, 0);
    gtk_table_attach(scriptImportWidget, (GtkWidget*)sw2, 0,1,1,2,  Opt, Opt, 0, 0);
    gtk_container_add((GtkContainer*)sw1, (GtkWidget*)import_treeview1);
    gtk_container_add((GtkContainer*)sw2, (GtkWidget*)import_treeview2);

    gtk_tree_view_set_model(import_treeview1, (GtkTreeModel*)import_liststore1);
    gtk_tree_view_set_model(import_treeview2, (GtkTreeModel*)import_liststore2);

    auto renderer1 = gtk_cell_renderer_text_new();
    auto renderer2 = gtk_cell_renderer_text_new();
    auto column1 = gtk_tree_view_column_new_with_attributes ("File scripts", renderer1, "text", 0, NULL);
    auto column2 = gtk_tree_view_column_new_with_attributes ("Project scripts", renderer2, "text", 0, NULL);
    gtk_tree_view_append_column(import_treeview1, column1);
    gtk_tree_view_append_column(import_treeview2, column2);
    connect_signal<void>(import_treeview1, bind(&VRGuiScripts::on_diag_import_select_1, this), "cursor_changed");
    connect_signal<void>(import_treeview2, bind(&VRGuiScripts::on_diag_import_select_2, this), "cursor_changed");

    // documentation widget
    setEntryCallback("entry25", bind(&VRGuiScripts::on_doc_filter_edited, this), true);
    setEntryCallback("tentry1", bind(&VRGuiScripts::on_templ_filter_edited, this), true);
}

OSG_END_NAMESPACE;





