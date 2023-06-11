#include <OpenSG/OSGRenderAction.h>

#include "VRGuiScripts.h"
#include "VRGuiBits.h"
#include "VRGuiManager.h"
#include "VRGuiConsole.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRScene.h"
#include "core/scripting/VRScript.h"
#include "core/utils/toString.h"
#include "core/utils/VRTimer.h"
#include "core/utils/xml.h"

#include <iostream>

OSG_BEGIN_NAMESPACE;
using namespace std;

// --------------------------
// ---------Callbacks--------
// --------------------------

VRScriptPtr VRGuiScripts::getSelectedScript() {
    /*VRGuiTreeView tree_view("treeview5");
    if (!tree_view.hasSelection()) return 0;

    // get selected script
    string name = tree_view.getSelectedStringValue(0);*/
    auto scene = VRScene::getCurrent();
    if (scene == 0) return 0;
    VRScriptPtr script = scene->getScript(selected);
    return script;
}

VRGuiScripts::group* VRGuiScripts::getSelectedGroup() {
    //VRGuiTreeView tree_view("treeview5");
    //if (!tree_view.hasSelection()) return 0;

    // get selected
    //string name = tree_view.getSelectedStringValue(0);
    for (auto& g : groups) if (g.second.name == selected) return &g.second;
    return 0;
}

/*void VRGuiScripts::setGroupListRow(GtkTreeIter* itr, group& g) {
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
}*/

void VRGuiScripts::on_new_clicked() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    auto g = getSelectedGroup();
    int l,c;
    getLineFocus(l,c);
    auto s = scene->newScript("Script", "\timport VR\n\n\t");
    if (g) s->setGroup(g->name);
    updateList();
    focusScript(s->getName(), l,c);
}

VRGuiScripts::group::group() { static int i = 0; ID = i; i++; }

void VRGuiScripts::on_addSep_clicked() {
    VRScriptPtr script = getSelectedScript();
    group g;
    groups[g.ID] = g;
    int l,c;
    getLineFocus(l,c);
    updateList();
    if (script) focusScript(script->getName(), l,c);
}

void VRGuiScripts::on_save_clicked() {
    cout << "VRGuiScripts::on_save_clicked " << endl;
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    string core = editor->getCore();
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->updateScript(script->getName(), core);

    //setWidgetSensitivity("toolbutton7", false);

    saveScene();
}

void VRGuiScripts::on_import_clicked() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    /*gtk_list_store_clear(import_liststore1);
    gtk_list_store_clear(import_liststore2);
    GtkTreeIter row;
    for (auto script : scene->getScripts()) {
        gtk_list_store_append(import_liststore2, &row);
        gtk_list_store_set (import_liststore2, &row, 0, script.first.c_str(), -1);
    }

    cout << "VRGuiScripts::on_import_clicked " << scriptImportWidget << endl;
    VRGuiFile::clearFilter();
    VRGuiFile::addFilter("Project", 2, "*.xml", "*.pvr");
    VRGuiFile::addFilter("All", 1, "*");
    VRGuiFile::setWidget(scriptImportWidget, true, true);
    VRGuiFile::setCallbacks( bind(&VRGuiScripts::on_diag_import, this), function<void()>(), bind(&VRGuiScripts::on_diag_import_select, this));
    VRGuiFile::open("Import", "open", "Import script");*/
}

void VRGuiScripts::on_diag_import_select() {
    /*gtk_list_store_clear(import_liststore1);
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
    }*/
}

void VRGuiScripts::on_diag_import() {
    /*VRGuiTreeView tree_view((GtkWidget*)import_treeview1);
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
    updateList();*/
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

void VRGuiScripts::on_perf_toggled(bool b) {
    //doPerf = getToggleToolButtonState("toggletoolbutton1");
    doPerf = b;
}

void VRGuiScripts::on_pause_toggled(bool b) {
    //bool b = getToggleToolButtonState("toggletoolbutton2");
    VRScene::getCurrent()->pauseScripts(b);
}

void VRGuiScripts::on_del_clicked() {
    /*VRGuiTreeView tree_view("treeview5");
    if (!tree_view.hasSelection()) return;

    // get selected script
    string name = tree_view.getSelectedStringValue(0);*/
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    VRScriptPtr script = scene->getScript(selected);
    if (script == 0) return;

    string msg1 = "Delete script " + selected;
    //if (!askUser(msg1, "Are you sure you want to delete this script?")) return;

    scene->remScript(script->getName());
    updateList();

    //setWidgetSensitivity("toolbutton9", false);
    //setWidgetSensitivity("toolbutton8", false);
}

void VRGuiScripts::on_select_script(string scriptName) { // selected a script
    if (pages.count(selected)) {
        auto& P = pages[selected];
        getLineFocus(P.line, P.column);
        cout << "editor deselect " << selected << ", cursor at: " << selected << "  " << P.line << "  " << P.column << endl;
    }

    selected = scriptName;
    auto script = getSelectedScript();
    if (!script) {
        // TODO: deactivate editor
        return;
    }
    trigger_cbs = false;

    // update options
    //setCombobox("combobox1", getListStorePos("liststore6", script->getType()));
    //auto setup = VRSetup::getCurrent();
    //if (setup) fillStringListstore("liststore7", setup->getDevices("server"));

    // update editor content
    editor->setCore(script->getScript(), script->getHeadSize());
    uiSignal("script_editor_set_parameters", {{"type",script->getType()},{"group",script->getGroup()}});

    uiSignal("script_editor_clear_trigs_and_args");
    // update arguments liststore
    for (auto a : script->getArguments()) uiSignal("script_editor_add_argument", {{"name",a->getName()},{"type",a->type},{"value",a->val}});

    // update trigger liststore
    for (auto t : script->getTriggers()) {
        string key = toString(t->key);
        if (t->dev == "keyboard" && t->key > 32 && t->key < 127) {
            char kc = t->key;
            key = kc;
        }
        uiSignal("script_editor_add_trigger", {
            {"name",t->getName()},
            {"trigger",t->trigger},
            {"parameter",t->param},
            {"device",t->dev},
            {"key",key},
            {"state",t->state}
        });
    }

    /*setWidgetSensitivity("toolbutton8", true);
    setWidgetSensitivity("toolbutton7", false);
    setWidgetSensitivity("toolbutton9", true);
    setWidgetSensitivity("table15", true);*/

    // language
    editor->setLanguage(script->getType());

    // script trigger
    //string trigger = script->getTrigger();
    //setTextEntry("entry48", script->getTriggerParams());

    //setCombobox("combobox1", getListStorePos("ScriptTrigger", trigger));
    trigger_cbs = true;

    if (pages.count(selected)) {
        pagePos P2 = pages[selected];
        if (P2.line > 0) {
            cout << " fokus selected " << selected << " " << P2.line << ", " << P2.column << endl;
            editor->focus(P2.line, P2.column);
        }
    }
}

// keyboard key detection
bool wait_for_key = false;
//GtkTreeIter trigger_row;
void VRGuiScripts::on_select_trigger() {
    wait_for_key = false;

    //VRGuiTreeView tree_view("treeview14");
    //if (!tree_view.hasSelection()) return;

    //tree_view.getSelection(&trigger_row);

    //pixbuf pressed?
    //string col_name = tree_view.getSelectedColumnName();
    //if (col_name != " ") return;

    // get key
    //string device = tree_view.getSelectedStringValue(1);
    //if (device != "keyboard") return;
    wait_for_key = true;
}

/*bool VRGuiScripts::on_any_key_event(GdkEventKey* event) {
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
}*/

void VRGuiScripts::on_rename_script(string new_name) {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    auto script = scene->getScript(selected);
    if (script == 0) return;
    auto s = scene->changeScriptName(selected, new_name);
    new_name = s->getName();
}

void VRGuiScripts::on_rename_group(string new_name) {
    for (auto& g : groups) {
        if (g.second.name != selected) continue;
        selected = new_name;
        g.second.name = new_name;
        for (auto& sw : g.second.scripts) if (auto s = sw.lock()) s->setGroup(new_name);
        updateList();
        //on_select_script();
        return;
    }
}

void VRGuiScripts::on_buffer_changed() {
    //setWidgetSensitivity("toolbutton7", true);

    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    // TODO
    // get in which line the changed occured
    // negate change if in line 0

    string core = editor->getCore();
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->updateScript(script->getName(), core, false);
}

//void VRGuiScripts::on_focus_out_changed(GdkEvent*) {}

shared_ptr<VRGuiEditor> VRGuiScripts::getEditor() { return editor; }

void VRGuiScripts::on_argadd_clicked() {
    cout << "VRGuiScripts::on_argadd_clicked " << endl;
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    script->addArgument();
    on_select_script(selected);
    on_save_clicked();
}

void VRGuiScripts::on_trigadd_clicked() {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    script->addTrigger();
    on_select_script(selected);
    on_save_clicked();
}

void VRGuiScripts::on_argrem_clicked(string aID) {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    script->remArgument(aID);
    on_select_script(selected);
    on_save_clicked();
}

void VRGuiScripts::on_trigrem_clicked(string tID) {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    script->remTrigger(tID);
    on_select_script(selected);
    on_save_clicked();
}

void VRGuiScripts::on_argname_edited(string name, string new_name) {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    // update argument name
    script->changeArgName(name, new_name);
    on_select_script(selected);
    on_save_clicked();
}

void VRGuiScripts::on_argval_edited(string name, string new_val) {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    // update argument name
    script->changeArgValue(name, new_val);
    on_select_script(selected);
    on_save_clicked();
}

void VRGuiScripts::on_argtype_edited(string name, string new_type) {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    script->changeArgType(name, new_type);
    on_select_script(selected);
    on_save_clicked();
}

void VRGuiScripts::on_trigger_edited(string name, string new_val) {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;
    cout << "VRGuiScripts::on_trigger_edited " << name << " " << new_val << endl;
    script->changeTrigger(name, new_val);
    on_select_script(selected);
    on_save_clicked();
}

void VRGuiScripts::on_trigdev_edited(string name, string new_val) {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    script->changeTrigDev(name, new_val);
    on_select_script(selected);
    on_save_clicked();
}

void VRGuiScripts::on_trigparam_edited(string name, string new_param) {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    script->changeTrigParams(name, new_param);
    on_select_script(selected);
    on_save_clicked();
}

void VRGuiScripts::on_trigkey_edited(string name, string new_key) {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    script->changeTrigKey(name, toInt(new_key));
    on_select_script(selected);
    on_save_clicked();
}

void VRGuiScripts::on_trigstate_edited(string name, string new_val) {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    script->changeTrigState(name, new_val);
    on_select_script(selected);
    on_save_clicked();
}

// templates dialog

void VRGuiScripts::on_template_clicked() {
    /*VRGuiScripts::updateTemplates();

    auto tb  = VRGuiBuilder::get()->get_object("scripttemplates");
    gtk_text_buffer_set_text((GtkTextBuffer*)tb, "", 0);

    showDialog("scriptTemplates");
    VRGuiWidget("tentry1").grabFocus();*/
}

void VRGuiScripts::updateTemplates() {
    /*auto store = (GtkTreeStore*)VRGuiBuilder::get()->get_object("templates");
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

    if (templ_filter != "") VRGuiTreeView("ttreeview1").expandAll();*/
}

void VRGuiScripts::on_select_templ() {
    /*VRGuiTreeView tree_view("ttreeview1");
    if (!tree_view.hasSelection()) return;

    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    // get selected object
    string templ  = tree_view.getSelectedStringValue(0);

    string core = scene->getTemplateCore(templ);

    auto tb = (GtkTextBuffer*)VRGuiBuilder::get()->get_object("scripttemplates");
    gtk_text_buffer_set_text(tb, core.c_str(), core.size());*/
}

/*void VRGuiScripts::on_doubleclick_templ(GtkTreePath* p, GtkTreeViewColumn* c) {
    //on_templ_import_clicked();
}*/

void VRGuiScripts::on_templ_close_clicked() { /*hideDialog("scriptTemplates");*/ }

void VRGuiScripts::on_templ_import_clicked() {
    /*hideDialog("scriptTemplates");

    VRGuiTreeView tree_view("ttreeview1");
    if (!tree_view.hasSelection()) return;

    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    // get selected object
    string templ = tree_view.getSelectedStringValue(0);
    scene->importTemplate(templ);
    updateList();*/
}

void VRGuiScripts::on_templ_filter_edited() {
    //templ_filter = getTextEntry("tentry1");
    updateTemplates();
}

// help dialog

void VRGuiScripts::on_help_close_clicked() {
    //hideDialog("pybindings-docs");
}

void VRGuiScripts::on_help_clicked() {
    //string txt = editor->getSelection();
    //openHelp(txt);
}

void VRGuiScripts::openHelp(string search) {
    /*VRGuiScripts::updateDocumentation();
    setTextEntry("entry25", search);
    showDialog("pybindings-docs");
    VRGuiWidget("entry25").grabFocus();*/
}

void VRGuiScripts::updateDocumentation() {
    /*auto store = (GtkTreeStore*)VRGuiBuilder::get()->get_object("bindings");
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

    if (docs_filter != "") VRGuiTreeView("treeview3").expandAll();*/
}

void VRGuiScripts::on_select_help() {
    /*VRGuiTreeView tree_view("treeview3");
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
    }*/
}

void VRGuiScripts::on_doc_filter_edited() {
    //docs_filter = getTextEntry("entry25");
    //updateDocumentation();
}

// script search dialog

void VRGuiScripts::on_find_clicked() {
    /*setToggleButton("checkbutton38", true);
    setWidgetSensitivity("entry11", false);
    string txt = editor->getSelection();
    setTextEntry("entry10", txt);
    VRGuiWidget("entry10").grabFocus();
    showDialog("find_dialog");*/
}

void VRGuiScripts::on_find_diag_cancel_clicked() {
    //hideDialog("find_dialog");
}

VRGuiScripts::searchResult::searchResult(string s, int l, int c) : scriptName(s), line(l), column(c) {}

void VRGuiScripts::selectScript(string name) {
    /*auto store = (GtkTreeStore*)VRGuiBuilder::get()->get_object("script_tree");
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
    selectScript2();*/
}

void VRGuiScripts::focusScript(string name, int line, int column) {
    /*setNotebookPage("notebook1", 2);
    setNotebookPage("notebook3", 2);
    selectScript(name); // messes with editor cursor position etc.. queueJob fixes it

    auto focusLine = [](shared_ptr<VRGuiEditor> editor, int line, int column) {
        editor->grabFocus();
        editor->setCursorPosition(line, column);
    };

    auto fkt = VRUpdateCb::create("gui_focus_script", bind(focusLine, editor, line, column));
    VRSceneManager::get()->queueJob(fkt, 0);*/
}

void VRGuiScripts::getLineFocus(int& line, int& column) {
    editor->getCursorPosition(line, column);
    line++;
    column++;
}

void VRGuiScripts::on_search_link_clicked(searchResult res, string s) {
    focusScript(res.scriptName, res.line, res.column);
}

void VRGuiScripts::on_find_diag_find_clicked() {
    /*bool sa = getCheckButtonState("checkbutton38");
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
        VRConsoleWidget::get( "Search results" )->write( m, style, link );
    };

    VRConsoleWidget::get( "Search results" )->addStyle( "blueLink", "#3355ff", "#ffffff", false, true, true, false );

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
    updateList();*/
}

void VRGuiScripts::on_toggle_find_replace() {
    //setWidgetSensitivity("entry11", getCheckButtonState("checkbutton12") );
    //setWidgetSensitivity("entry11", false);
    //setTextEntry("entry11", "Coming soon!");
}

// config stuff

void VRGuiScripts::on_change_script_type(string type) {
    if(!trigger_cbs) return;
    VRScriptPtr script = getSelectedScript();
    if (!script) return;
    script->setType(type);
    on_select_script(selected);
    on_save_clicked();
}

void VRGuiScripts::on_change_group(string group) {
    if(!trigger_cbs) return;
    VRScriptPtr script = getSelectedScript();
    if (!script) return;
    int l,c;
    getLineFocus(l,c);
    script->setGroup( group );
    on_select_script(selected);
    on_save_clicked();
    updateList();
    focusScript(script->getName(), l,c);
}

void VRGuiScripts::on_change_server() {
    if(!trigger_cbs) return;
    VRScriptPtr script = getSelectedScript();
    if (!script) return;
    //script->setHTMLHost( getComboboxText("combobox24") );
    on_select_script(selected);
    on_save_clicked();
}

// others

void VRGuiScripts::on_convert_cpp_clicked() {
    VRScriptPtr script = getSelectedScript();
    if (!script) return;

    auto countTabs = [](const string& line) {
        int i=0;
        while (line[i] == '\t') i++;
        return i;
    };

    auto countTrailingEmptyChars = [](const string& txt, int i0) {
        int i=0;
        while (txt[i0-i] == '\n' || txt[i0-i] == '\t') i++;
        return i;
    };

    auto findAndReplace = [](string& line, string s1, string s2) {
        int p = line.find(s1);
        while(p != string::npos) {
            line.replace(p, s1.size(), s2);
            p = line.find(s1);
        }
    };

    const string core = script->getHead() + script->getCore();
    string newCore = "";

    int lastTabCount = 0;
    bool lastLineEmpty = 0;
    size_t k1 = 0;
    size_t k2 = 0;

    map<string, string> lineStarts;
    lineStarts["def "] = "void ";
    lineStarts["if "] = "if (";
    lineStarts["for "] = "for (";
    lineStarts["while "] = "while (";

    auto lines = splitString(core, '\n');
    for (int i=0; i<lines.size(); i++) {
        string& line = lines[i];
        int N1 = line.size();
        int N2 = line.size();

        bool skipLine = false;
        bool addSemicolon = true;
        bool emptyLine = false;
        int tabCount = countTabs(line);

        if ( N2-tabCount == 0 ) emptyLine = true;
        if ( emptyLine ) addSemicolon = false;

        if (line[N2-1] == ':') {
            line[N2-1] = ' ';
            if (line[N2-1] == ')') line += "{";
            else line += ") {";

            N2 = line.size();
            addSemicolon = false;
        }

        for (auto ls : lineStarts) {
            int k = ls.first.size();
            if ( subString(line, tabCount, k) == ls.first ) {
                line = line.replace(tabCount, k, ls.second);
                N2 = line.size();

                if (ls.first != "def ") {
                    int p = line.find(':');
                    if (p != string::npos) line[p] = ')';
                }

                if (ls.first == "for ") findAndReplace(line, " in ", " : ");
            }
        }

        findAndReplace(line, "#", "; //");
        findAndReplace(line, "(self, ", "(");
        findAndReplace(line, "self.", "");
        findAndReplace(line, "VR.", "");
        findAndReplace(line, ".values()", "");
        findAndReplace(line, ".items()", "");
        findAndReplace(line, ".", "->");
        findAndReplace(line, "'", "\"");
        findAndReplace(line, " and ", " && ");
        findAndReplace(line, " or ", " || ");
        if (contains(line, "hasattr")) line = "//"+line;

        if (addSemicolon) line += ";";

        // find and replace "VR." with "VR"

        if (!emptyLine && tabCount<lastTabCount) { // closing brackets
            int N = newCore.size()-1;
            int nec = countTrailingEmptyChars(newCore, N);
            string whitespace = subString(newCore, N-nec, nec);
            newCore = subString(newCore, 0, N+1-nec);
            for (int t=lastTabCount; t>tabCount; t--) {
                newCore += "\n"+string(t-1, '\t') + "}";
            }
            newCore += whitespace + "\n";
        }

        if (!emptyLine) lastTabCount = tabCount;
        lastLineEmpty = emptyLine;
        k1 += N1;
        k2 += N2;

        if (!skipLine) newCore += line+"\n";
    }

    int nec = countTrailingEmptyChars(newCore, newCore.size()-1);
    newCore = subString(newCore, 0, newCore.size()-nec);
    if (newCore[newCore.size()-1] != '}') newCore += "\n}";
    VRConsoleWidget::get( "Console" )->write( newCore );
}

// --------------------------
// ---------Main-------------
// --------------------------

bool VRGuiScripts::on_scene_changed() {
	cout << "VRGuiScripts::on_scene_changed" << endl;
    groups.clear();
    return true;
}

void VRGuiScripts::update() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    //for (auto& r : scriptRows) setScriptListRow(&r.second, r.first.lock(), true);
}

bool VRGuiScripts::updateList() {
	cout << "VRGuiScripts::updateList" << endl;
    auto scene = VRScene::getCurrent();
    if (scene == 0) return true;

    uiSignal("scripts_list_clear");

    auto oldpages = pages;
    pages.clear();

    map<string, int> grpIter;
    auto addGroupRow = [&](group& g) {
        uiSignal("scripts_list_add_group", {{"name",g.name},{"ID",toString(g.ID)}});
        grpIter[g.name] = g.ID;
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

    //scriptRows.clear();
    for (auto script : scene->getScripts()) {
        auto k = script.second.get();
        string grp = script.second->getGroup();
        string name = script.second->getName();
        if (!grpIter.count(grp)) uiSignal("scripts_list_add_script", {{"name",name},{"group",""}});
        else {
            auto& g = groups[grpIter[grp]];
            g.scripts.push_back(script.second);
            uiSignal("scripts_list_add_script", {{"name",name},{"group",toString(g.ID)}});
        }
        //scriptRows.push_back( pair<VRScriptPtr, GtkTreeIter>(s,itr) );
        //setScriptListRow(&itr, s);

        if (oldpages.count(name)) pages[name] = oldpages[name];
        else pages[name] = pagePos();
    }
    on_select_script(selected);

    if (selected == "") {
        cout << "No script open, selecting a script.." << endl;
        if (scene->getScript("init")) selectScript("init");
        else {
            auto scs = scene->getScripts();
            if (scs.size() > 0) {
                auto sc = scs.begin()->second;
                if (sc) selectScript(sc->getName());
            }
        }
    }

    return true;
}

/*bool VRGuiScripts_on_editor_select(GtkWidget* widget, GdkEvent* event, VRGuiScripts* self) {
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
}*/

/*bool VRGuiScripts::on_help_close_frame_clicked(GdkEvent* event) {
    auto diag = VRGuiBuilder::get()->get_widget("pybindings-docs");
    gtk_widget_hide(diag);
    return true;
}*/

namespace PL = std::placeholders;

VRGuiScripts::VRGuiScripts() {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("select_script", [&](OSG::VRGuiSignals::Options o) { on_select_script(o["script"]); return true; } );
    mgr->addCallback("select_group", [&](OSG::VRGuiSignals::Options o) { on_select_script(o["group"]); return true; } );
    mgr->addCallback("rename_script", [&](OSG::VRGuiSignals::Options o) { on_rename_script(o["name"]); return true; } );
    mgr->addCallback("rename_group", [&](OSG::VRGuiSignals::Options o) { on_rename_group(o["name"]); return true; } );
    mgr->addCallback("scripts_toolbar_new", [&](OSG::VRGuiSignals::Options o) { on_new_clicked(); return true; }, true );
    mgr->addCallback("scripts_toolbar_template", [&](OSG::VRGuiSignals::Options o) { on_template_clicked(); return true; } );
    mgr->addCallback("scripts_toolbar_group", [&](OSG::VRGuiSignals::Options o) { on_addSep_clicked(); return true; } );
    mgr->addCallback("scripts_toolbar_import", [&](OSG::VRGuiSignals::Options o) { on_import_clicked(); return true; } );
    mgr->addCallback("scripts_toolbar_delete", [&](OSG::VRGuiSignals::Options o) { on_del_clicked(); return true; } );
    mgr->addCallback("scripts_toolbar_pause", [&](OSG::VRGuiSignals::Options o) { on_pause_toggled(toBool(o["state"])); return true; } );
    //mgr->addCallback("scripts_toolbar_cpp", [&](OSG::VRGuiSignals::Options o) { on_exec_clicked(); return true; } );
    mgr->addCallback("scripts_toolbar_save", [&](OSG::VRGuiSignals::Options o) { on_save_clicked(); return true; }, true );
    mgr->addCallback("scripts_toolbar_execute", [&](OSG::VRGuiSignals::Options o) { on_exec_clicked(); return true; }, true );
    //mgr->addCallback("scripts_toolbar_search", [&](OSG::VRGuiSignals::Options o) { on_exec_clicked(); return true; } );
    mgr->addCallback("scripts_toolbar_documentation", [&](OSG::VRGuiSignals::Options o) { on_help_clicked(); return true; } );
    mgr->addCallback("scripts_toolbar_performance", [&](OSG::VRGuiSignals::Options o) { on_perf_toggled(toBool(o["state"])); return true; } );
    mgr->addCallback("script_editor_text_changed", [&](OSG::VRGuiSignals::Options o) { on_buffer_changed(); return true; } );

    mgr->addCallback("script_editor_change_type", [&](OSG::VRGuiSignals::Options o) { on_change_script_type(o["type"]); return true; }, true );
    mgr->addCallback("script_editor_change_group", [&](OSG::VRGuiSignals::Options o) { on_change_group(o["group"]); return true; }, true );

    mgr->addCallback("script_editor_new_trigger", [&](OSG::VRGuiSignals::Options o) { on_trigadd_clicked(); return true; }, true );
    mgr->addCallback("script_editor_new_argument", [&](OSG::VRGuiSignals::Options o) { on_argadd_clicked(); return true; }, true );
    mgr->addCallback("script_editor_rem_trigger", [&](OSG::VRGuiSignals::Options o) { on_trigrem_clicked(o["trigger"]); return true; }, true );
    mgr->addCallback("script_editor_rem_argument", [&](OSG::VRGuiSignals::Options o) { on_argrem_clicked(o["argument"]); return true; }, true );

    mgr->addCallback("script_editor_rename_argument", [&](OSG::VRGuiSignals::Options o) { on_argname_edited(o["inputOld"], o["inputNew"]); return true; }, true );
    mgr->addCallback("script_editor_change_argument", [&](OSG::VRGuiSignals::Options o) { on_argval_edited(o["idKey"], o["inputNew"]); return true; }, true );
    mgr->addCallback("script_editor_change_argument_type", [&](OSG::VRGuiSignals::Options o) { on_argtype_edited(o["idKey"], o["newValue"]); return true; }, true );

    mgr->addCallback("script_editor_change_trigger_type", [&](OSG::VRGuiSignals::Options o) { on_trigger_edited(o["idKey"], o["newValue"]); return true; }, true );
    mgr->addCallback("script_editor_change_trigger_param", [&](OSG::VRGuiSignals::Options o) { on_trigparam_edited(o["idKey"], o["inputNew"]); return true; }, true );
    mgr->addCallback("script_editor_change_trigger_device", [&](OSG::VRGuiSignals::Options o) { on_trigdev_edited(o["idKey"], o["newValue"]); return true; }, true );
    mgr->addCallback("script_editor_change_trigger_key", [&](OSG::VRGuiSignals::Options o) { on_trigkey_edited(o["idKey"], o["inputNew"]); return true; }, true );
    mgr->addCallback("script_editor_change_trigger_state", [&](OSG::VRGuiSignals::Options o) { on_trigstate_edited(o["idKey"], o["newValue"]); return true; }, true );

    /*disableDestroyDiag("pybindings-docs");
    disableDestroyDiag("find_dialog");
    disableDestroyDiag("scriptTemplates");

    setToolButtonCallback("toolbutton6", bind(&VRGuiScripts::on_new_clicked, this) );
    setToolButtonCallback("toolbutton29", bind(&VRGuiScripts::on_template_clicked, this) );
    setToolButtonCallback("toolbutton7", bind(&VRGuiScripts::on_save_clicked, this) );
    setToolButtonCallback("toolbutton8", bind(&VRGuiScripts::on_exec_clicked, this) 888);
    setToolButtonCallback("toolbutton9", bind(&VRGuiScripts::on_del_clicked, this) );
    setToolButtonCallback("toolbutton16", bind(&VRGuiScripts::on_help_clicked, this) );
    setToolButtonCallback("toolbutton20", bind(&VRGuiScripts::on_addSep_clicked, this) );
    setToolButtonCallback("toolbutton22", bind(&VRGuiScripts::on_import_clicked, this) );
    setToolButtonCallback("toolbutton23", bind(&VRGuiScripts::on_find_clicked, this) );
    setToolButtonCallback("toggletoolbutton1", bind(&VRGuiScripts::on_perf_toggled, this) );
    setToolButtonCallback("toggletoolbutton2", bind(&VRGuiScripts::on_pause_toggled, this) );
    setToolButtonCallback("toolbutton30", bind(&VRGuiScripts::on_convert_cpp_clicked, this) );

    setButtonCallback("button16", bind(&VRGuiScripts::on_help_close_clicked, this) );
    setButtonCallback("tbutton1", bind(&VRGuiScripts::on_templ_close_clicked, this) );
    setButtonCallback("tbutton2", bind(&VRGuiScripts::on_templ_import_clicked, this) );
    setButtonCallback("button28", bind(&VRGuiScripts::on_find_diag_cancel_clicked, this) );
    setButtonCallback("button29", bind(&VRGuiScripts::on_find_diag_find_clicked, this) );

    setToggleButtonCallback("checkbutton12", bind(&VRGuiScripts::on_toggle_find_replace, this) );

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
    setTreeviewDoubleclickCallback("ttreeview1", bind(&VRGuiScripts::on_doubleclick_templ, this, PL::_1, PL::_2) );*/

    editor = shared_ptr<VRGuiEditor>( new VRGuiEditor("scrolledwindow4") );
    editor->addKeyBinding("find", VRUpdateCb::create("findCb", bind(&VRGuiScripts::on_find_clicked, this)));
    editor->addKeyBinding("help", VRUpdateCb::create("helpCb", bind(&VRGuiScripts::on_help_clicked, this)));
    editor->addKeyBinding("save", VRUpdateCb::create("saveCb", bind(&VRGuiScripts::on_save_clicked, this)));
    editor->addKeyBinding("exec", VRUpdateCb::create("execCb", bind(&VRGuiScripts::on_exec_clicked, this)));
    /*connect_signal<void, GdkEvent*>(editor->getEditor(), bind(&VRGuiScripts::on_focus_out_changed, this, PL::_1), "focus-out-event");

    setEntryCallback("entry10", bind(&VRGuiScripts::on_find_diag_find_clicked, this), false, false);

    setCellRendererCallback("cellrenderertext13", bind(&VRGuiScripts::on_name_edited, this, PL::_1, PL::_2) );
    setCellRendererCallback("cellrenderertext16", bind(&VRGuiScripts::on_trigparam_edited, this, PL::_1, PL::_2) );
    setCellRendererCallback("cellrenderertext41", bind(&VRGuiScripts::on_trigkey_edited, this, PL::_1, PL::_2) );

    setCellRendererCombo("treeviewcolumn27", "ScriptTrigger", 0, bind(&VRGuiScripts::on_trigger_edited, this, PL::_1, PL::_2) );
    setCellRendererCombo("treeviewcolumn28", "ScriptTriggerDevices", 1, bind(&VRGuiScripts::on_trigdev_edited, this, PL::_1, PL::_2) );
    setCellRendererCombo("treeviewcolumn30", "ScriptTriggerStates", 3, bind(&VRGuiScripts::on_trigstate_edited, this, PL::_1, PL::_2) );*/

    // fill combolists
    const char *arg_types[] = {"int", "float", "str", "VRPyObjectType", "VRPyTransformType", "VRPyGeometryType", "VRPyLightType", "VRPyLodType", "VRPyDeviceType", "VRPyMouseType", "VRPyHapticType", "VRPySocketType", "VRPyLeapFrameType"};
    const char *trigger_types[] = {"none", "on_scene_load", "on_scene_close", "on_scene_import", "on_timeout", "on_device", "on_socket"};
    const char *device_types[] = {"mouse", "multitouch", "keyboard", "flystick", "haptic", "server1", "leap", "vrpn_device"}; // TODO: get from a list in devicemanager or something
    const char *trigger_states[] = {"Pressed", "Released", "Drag", "Drop", "To edge", "From edge"};
    const char *script_types[] = {"Python", "GLSL", "HTML"};
    /*fillStringListstore("arg_types", vector<string>(arg_types, end(arg_types)) );
    fillStringListstore("ScriptTrigger", vector<string>(trigger_types, end(trigger_types)) );
    fillStringListstore("ScriptTriggerDevices", vector<string>(device_types, end(device_types)) );
    fillStringListstore("ScriptTriggerStates", vector<string>(trigger_states, end(trigger_states)) );
    fillStringListstore("liststore6", vector<string>(script_types, end(script_types)) );

    setWidgetSensitivity("toolbutton7", false);
    setWidgetSensitivity("toolbutton8", false);
    setWidgetSensitivity("toolbutton9", false);*/

    // update the list each frame to update the execution time
    updatePtr = VRUpdateCb::create("scripts_gui_update",  bind(&VRGuiScripts::update, this) );
    VRSceneManager::get()->addUpdateFkt(updatePtr, 100);

    sceneChangedCb = VRDeviceCb::create("GUI_sceneChanged", bind(&VRGuiScripts::on_scene_changed, this) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( sceneChangedCb );

    // init scriptImportWidget
    /*scriptImportWidget = (GtkGrid*)gtk_grid_new();
    GtkScrolledWindow* sw1 = (GtkScrolledWindow*)gtk_scrolled_window_new(0,0);
    GtkScrolledWindow* sw2 = (GtkScrolledWindow*)gtk_scrolled_window_new(0,0);
    import_treeview1 = (GtkTreeView*)gtk_tree_view_new();
    import_treeview2 = (GtkTreeView*)gtk_tree_view_new();

    import_liststore1 = gtk_list_store_new(9, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    import_liststore2 = gtk_list_store_new(9, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

    auto line = gtk_separator_new(GTK_ORIENTATION_VERTICAL);

    gtk_grid_attach(scriptImportWidget, (GtkWidget*)sw2, 0,0,1,1);
    gtk_grid_attach(scriptImportWidget, (GtkWidget*)line, 1,0,1,1);
    gtk_grid_attach(scriptImportWidget, (GtkWidget*)sw1, 2,0,1,1);
    gtk_widget_set_vexpand((GtkWidget*)sw1, true);
    gtk_widget_set_hexpand((GtkWidget*)sw1, true);
    gtk_widget_set_vexpand((GtkWidget*)line, true);
    gtk_widget_set_vexpand((GtkWidget*)sw2, true);
    gtk_widget_set_hexpand((GtkWidget*)sw2, true);
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
    setEntryCallback("tentry1", bind(&VRGuiScripts::on_templ_filter_edited, this), true);*/
}

OSG_END_NAMESPACE;





