#include "VRGuiScripts.h"
#include "VRGuiUtils.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRScene.h"
#include "core/scripting/VRScript.h"
#include "core/utils/toString.h"
#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/textview.h>
#include <gtkmm/combobox.h>
#include <gtkmm/dialog.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treestore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguagemanager.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

GtkSourceBuffer* VRGuiScripts_sourceBuffer;

class VRGuiScripts_ModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiScripts_ModelColumns() { add(script); }
        Gtk::TreeModelColumn<Glib::ustring> script;
};

class VRGuiScripts_ArgsModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiScripts_ArgsModelColumns() { add(name); add(value); add(obj); add(type); }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> value;
        Gtk::TreeModelColumn<gpointer> obj;
        Gtk::TreeModelColumn<Glib::ustring> type;
};

class VRGuiScripts_TrigsModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiScripts_TrigsModelColumns() { add(trigs); add(devs); add(key); add(state); add(params); add(obj); add(name); }
        Gtk::TreeModelColumn<Glib::ustring> trigs;
        Gtk::TreeModelColumn<Glib::ustring> devs;
        Gtk::TreeModelColumn<Glib::ustring> key;
        Gtk::TreeModelColumn<Glib::ustring> state;
        Gtk::TreeModelColumn<Glib::ustring> params;
        Gtk::TreeModelColumn<gpointer> obj;
        Gtk::TreeModelColumn<Glib::ustring> name;
};

class VRGuiScripts_HelpColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiScripts_HelpColumns() { add(obj); add(type); add(mod); }
        Gtk::TreeModelColumn<Glib::ustring> obj;
        Gtk::TreeModelColumn<Glib::ustring> type;
        Gtk::TreeModelColumn<Glib::ustring> mod;
};

// --------------------------
// ---------Callbacks--------
// --------------------------

string VRGuiScripts::get_editor_core(int i) {
    GtkTextIter itr_s, itr_e;
    gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(VRGuiScripts_sourceBuffer), &itr_s);
    gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(VRGuiScripts_sourceBuffer), &itr_e);
    for (int j=0; j<i; j++) gtk_text_iter_forward_line(&itr_s);// skip head
    return string( gtk_text_buffer_get_text( GTK_TEXT_BUFFER(VRGuiScripts_sourceBuffer), &itr_s, &itr_e, true) );
}

VRScript* VRGuiScripts::getSelectedScript() {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview5"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return 0;

    // get selected script
    VRGuiScripts_ModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.script);
    VRScript* script = VRSceneManager::getCurrent()->getScript(name);

    return script;
}

void VRGuiScripts::on_new_clicked() {
    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("script_list"));
    Gtk::ListStore::Row row = *store->append();

    VRScript* script = VRSceneManager::getCurrent()->newScript("Script", "\tpass");
    gtk_list_store_set (store->gobj(), row.gobj(), 0, script->getName().c_str(), -1);
}

void VRGuiScripts::on_save_clicked() {
    VRScript* script = VRGuiScripts::getSelectedScript();
    if (script == 0) return;

    string core = VRGuiScripts::get_editor_core(script->getHeadSize());
    VRSceneManager::getCurrent()->updateScript(script->getName(), core);

    setToolButtonSensivity("toolbutton7", false);

    saveScene();
}

void VRGuiScripts::on_exec_clicked() {
    VRScript* script = VRGuiScripts::getSelectedScript();
    if (script == 0) return;

    on_save_clicked();

    VRSceneManager::getCurrent()->triggerScript(script->getName());

    VRGuiSignals::get()->getSignal("scene_modified")->trigger();
}

void VRGuiScripts::on_del_clicked() {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview5"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // get selected script
    VRGuiScripts_ModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.script);
    VRScript* script = VRSceneManager::getCurrent()->getScript(name);
    if (script == 0) return;

    string msg1 = "Delete script " + name;
    if (!askUser(msg1, "Are you sure you want to delete this script?")) return;

    VRSceneManager::getCurrent()->remScript(script->getName());

    Glib::RefPtr<Gtk::ListStore> list_store  = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("script_list"));
    list_store->erase(iter);

    setToolButtonSensivity("toolbutton9", false);
    setToolButtonSensivity("toolbutton8", false);
}

void VRGuiScripts::on_select_script() { // selected a script
    VRScript* script = VRGuiScripts::getSelectedScript();
    if (script == 0) {
        setToolButtonSensivity("toolbutton8", false);
        setToolButtonSensivity("toolbutton7", false);
        setToolButtonSensivity("toolbutton9", false);
        setTableSensivity("table15", false);
        return;
    }

    trigger_cbs = false;

    // update options
    setCombobox("combobox1", getListStorePos("liststore6", script->getType()));
    fillStringListstore("liststore7", VRSetupManager::get()->getCurrent()->getDevices("mobile"));
    setCombobox("combobox24", getListStorePos("liststore7", script->getMobile()));

    // update editor content and script head
    string core = script->getHead() + script->getCore();
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(VRGuiScripts_sourceBuffer), core.c_str(), core.size());

    // update arguments liststore
    Glib::RefPtr<Gtk::ListStore> args = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("liststore2"));
    args->clear();
    map<string, VRScript::arg*> arg_map = script->getArguments();
    map<string, VRScript::arg*>::iterator itr;
    if (PyErr_Occurred() != NULL) PyErr_Print();
    for (itr = arg_map.begin(); itr != arg_map.end(); itr++) {
        VRScript::arg* a = itr->second;
        Gtk::ListStore::Row row = *args->append();
        gtk_list_store_set(args->gobj(), row.gobj(), 0, a->getName().c_str(), -1);
        gtk_list_store_set(args->gobj(), row.gobj(), 1, a->val.c_str(), -1);
        gtk_list_store_set(args->gobj(), row.gobj(), 2, script, -1);
        gtk_list_store_set(args->gobj(), row.gobj(), 3, a->type.c_str(), -1);
    }

    // update trigger liststore
    Glib::RefPtr<Gtk::ListStore> trigs = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("triggers"));
    trigs->clear();
    map<string, VRScript::trig*> trig_map = script->getTriggers();
    map<string, VRScript::trig*>::iterator itr2;
    if (PyErr_Occurred() != NULL) PyErr_Print();
    for (itr2 = trig_map.begin(); itr2 != trig_map.end(); itr2++) {
        VRScript::trig* t = itr2->second;
        string key = toString(t->key);
        if (t->dev == "keyboard" and t->key > 32 and t->key < 127) {
            char kc = t->key;
            key = kc;
        }
        Gtk::ListStore::Row row = *trigs->append();
        gtk_list_store_set(trigs->gobj(), row.gobj(), 0, t->trigger.c_str(), -1);
        gtk_list_store_set(trigs->gobj(), row.gobj(), 1, t->dev.c_str(), -1);
        gtk_list_store_set(trigs->gobj(), row.gobj(), 2, key.c_str(), -1);
        gtk_list_store_set(trigs->gobj(), row.gobj(), 3, t->state.c_str(), -1);
        gtk_list_store_set(trigs->gobj(), row.gobj(), 4, t->param.c_str(), -1);
        gtk_list_store_set(trigs->gobj(), row.gobj(), 5, script, -1);
        gtk_list_store_set(trigs->gobj(), row.gobj(), 6, t->getName().c_str(), -1);
    }

    setToolButtonSensivity("toolbutton8", true);
    setToolButtonSensivity("toolbutton7", false);
    setToolButtonSensivity("toolbutton9", true);
    setTableSensivity("table15", true);

    // script trigger
    //string trigger = script->getTrigger();
    //setTextEntry("entry48", script->getTriggerParams());

    //setCombobox("combobox1", getListStorePos("ScriptTrigger", trigger));
    trigger_cbs = true;
}

// keyboard key detection
bool wait_for_key = false;
Gtk::TreeModel::Row trigger_row;
void VRGuiScripts::on_select_trigger() {
    wait_for_key = false;

    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview14"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    //pixbuf pressed?
    Gtk::TreeModel::Path p;
    Gtk::TreeViewColumn* c;
    tree_view->get_cursor(p,c);
    string col_name = c->get_title();
    if (col_name != "") return;

    // get key
    VRGuiScripts_TrigsModelColumns cols;
    trigger_row = *iter;
    string device = trigger_row.get_value(cols.devs);
    if (device != "keyboard") return;

    wait_for_key = true;
}

bool VRGuiScripts::on_any_key_event(GdkEventKey* event) {
    if (!wait_for_key) return false;
    wait_for_key = false;

    int key = event->keyval;
    VRGuiScripts_TrigsModelColumns cols;
    trigger_row[cols.key] = toString(key);

    VRScript* script = (VRScript*)trigger_row.get_value(cols.obj);
    script->changeTrigKey(trigger_row.get_value(cols.name), key);
    on_select_script();
    on_save_clicked();

    return true;
}

bool VRGuiScripts::on_any_event(GdkEvent* event) {
    int t = event->type;
    if (t == 5 or t == 6 or t == 12) {
        //wait_for_key = false; // TODO
    }
    return false;
}


void VRGuiScripts::on_name_edited(const Glib::ustring& path, const Glib::ustring& new_name) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview5"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new name
    VRGuiScripts_ModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.script);
    row[cols.script] = new_name;

    // update key in map
    VRSceneManager::getCurrent()->changeScriptName(name, new_name);
    on_select_script();
}


void VRGuiScripts_on_script_changed(GtkTextBuffer* tb, gpointer user_data) {
    setToolButtonSensivity("toolbutton7", true);

    VRGuiScripts* gs = (VRGuiScripts*)user_data;

    VRScript* script = gs->getSelectedScript();
    if (script == 0) return;

    // TODO
    // get in which line the changed occured
    // negate change if in line 0

    string core = gs->get_editor_core(script->getHeadSize());
    VRSceneManager::getCurrent()->updateScript(script->getName(), core, false);
}


void VRGuiScripts::on_argadd_clicked() {
    VRScript* script = VRGuiScripts::getSelectedScript();
    if (script == 0) return;

    script->addArgument();
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_trigadd_clicked() {
    VRScript* script = VRGuiScripts::getSelectedScript();
    if (script == 0) return;

    script->addTrigger();
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_argrem_clicked() {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview7"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new name
    VRGuiScripts_ArgsModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.name);
    VRScript* script = (VRScript*)row.get_value(cols.obj);

    script->remArgument(name);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_trigrem_clicked() {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview14"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new name
    VRGuiScripts_TrigsModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.name);
    VRScript* script = (VRScript*)row.get_value(cols.obj);

    script->remTrigger(name);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_argname_edited(const Glib::ustring& path, const Glib::ustring& new_name) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview7"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new name
    VRGuiScripts_ArgsModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.name);
    row[cols.name] = new_name;
    VRScript* script = (VRScript*)row.get_value(cols.obj);

    // update argument name
    script->changeArgName(name, new_name);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_argval_edited(const Glib::ustring& path, const Glib::ustring& new_name) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview7"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new value
    VRGuiScripts_ArgsModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    //string value = row.get_value(cols.value);
    string name = row.get_value(cols.name);
    row[cols.value] = new_name;
    VRScript* script = (VRScript*)row.get_value(cols.obj);

    // update argument name
    script->changeArgValue(name, new_name);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_argtype_edited(const Glib::ustring& new_name, const Gtk::TreeModel::iterator& new_iter) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview7"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new type
    Glib::RefPtr<Gtk::ListStore> combo_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("arg_types"));
    gchar *t;
    gtk_tree_model_get((GtkTreeModel*)combo_list->gobj(), (GtkTreeIter*)new_iter->gobj(), 0, &t, -1);
    string type = string(t);
    Gtk::TreeModel::Row row = *iter;
    VRGuiScripts_ArgsModelColumns cols;
    row[cols.type] = type;

    // do something
    VRScript* script = (VRScript*)row.get_value(cols.obj);
    script->changeArgType(row.get_value(cols.name), type);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_trigger_edited(const Glib::ustring& new_name, const Gtk::TreeModel::iterator& new_iter) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview14"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new type
    Glib::RefPtr<Gtk::ListStore> combo_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("ScriptTrigger"));
    gchar *t;
    gtk_tree_model_get((GtkTreeModel*)combo_list->gobj(), (GtkTreeIter*)new_iter->gobj(), 0, &t, -1);
    string type = string(t);
    Gtk::TreeModel::Row row = *iter;
    VRGuiScripts_TrigsModelColumns cols;
    row[cols.trigs] = type;

    // do something
    VRScript* script = (VRScript*)row.get_value(cols.obj);
    script->changeTrigger(row.get_value(cols.name), type);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_trigdev_edited(const Glib::ustring& new_name, const Gtk::TreeModel::iterator& new_iter) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview14"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new type
    Glib::RefPtr<Gtk::ListStore> combo_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("ScriptTriggerDevices"));
    gchar *t;
    gtk_tree_model_get((GtkTreeModel*)combo_list->gobj(), (GtkTreeIter*)new_iter->gobj(), 0, &t, -1);
    string type = string(t);
    Gtk::TreeModel::Row row = *iter;
    VRGuiScripts_TrigsModelColumns cols;
    row[cols.devs] = type;

    // do something
    VRScript* script = (VRScript*)row.get_value(cols.obj);
    script->changeTrigDev(row.get_value(cols.name), type);
    on_select_script();
    on_save_clicked();
}


void VRGuiScripts::on_trigparam_edited(const Glib::ustring& path, const Glib::ustring& new_name) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview14"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new value
    VRGuiScripts_TrigsModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    row[cols.params] = new_name;

    // do something
    VRScript* script = (VRScript*)row.get_value(cols.obj);
    script->changeTrigParams(row.get_value(cols.name), new_name);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_trigkey_edited(const Glib::ustring& path, const Glib::ustring& new_name) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview14"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new value
    VRGuiScripts_TrigsModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    row[cols.key] = new_name;

    // do something
    VRScript* script = (VRScript*)row.get_value(cols.obj);
    script->changeTrigKey(row.get_value(cols.name), toInt(new_name));
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_trigstate_edited(const Glib::ustring& new_name, const Gtk::TreeModel::iterator& new_iter) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview14"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new type
    Glib::RefPtr<Gtk::ListStore> combo_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("ScriptTriggerStates"));
    gchar *t;
    gtk_tree_model_get((GtkTreeModel*)combo_list->gobj(), (GtkTreeIter*)new_iter->gobj(), 0, &t, -1);
    string type = string(t);
    Gtk::TreeModel::Row row = *iter;
    VRGuiScripts_TrigsModelColumns cols;
    row[cols.state] = type;

    // do something
    VRScript* script = (VRScript*)row.get_value(cols.obj);
    script->changeTrigState(row.get_value(cols.name), type);
    on_select_script();
    on_save_clicked();
}

// help dialog

void VRGuiScripts::on_help_close_clicked() {
    Gtk::Dialog* diag;
    VRGuiBuilder()->get_widget("dialog1", diag);
    diag->hide();
}

void VRGuiScripts::loadHelp() {
    Glib::RefPtr<Gtk::TreeStore> tree_store = Glib::RefPtr<Gtk::TreeStore>::cast_static(VRGuiBuilder()->get_object("bindings"));
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview6"));
    tree_store->clear();

    Gtk::TreeModel::iterator itr;
    Gtk::TreeModel::iterator itr2;
    Gtk::TreeStore::Row row;

    VRScriptManager* sm = VRSceneManager::getCurrent();
    vector<string> types = sm->getPyVRTypes();
    for (uint i=0; i<types.size(); i++) {
        itr = tree_store->append();
        row = *itr;
        gtk_tree_store_set (tree_store->gobj(), row.gobj(), 0, types[i].c_str(), -1);
        gtk_tree_store_set (tree_store->gobj(), row.gobj(), 1, "module", -1);
        gtk_tree_store_set (tree_store->gobj(), row.gobj(), 2, types[i].c_str(), -1);
        vector<string> methods = sm->getPyVRMethods(types[i]);
        for (uint j=0; j<methods.size(); j++) {
            itr2 = tree_store->append(itr->children());
            row = *itr2;
            gtk_tree_store_set (tree_store->gobj(), row.gobj(), 0, methods[j].c_str(), -1);
            gtk_tree_store_set (tree_store->gobj(), row.gobj(), 1, "method", -1);
            gtk_tree_store_set (tree_store->gobj(), row.gobj(), 2, types[i].c_str(), -1);
        }
    }
}

void VRGuiScripts::on_help_clicked() {
    VRGuiScripts::loadHelp();

    Glib::RefPtr<Gtk::TextBuffer> tb  = Glib::RefPtr<Gtk::TextBuffer>::cast_static(VRGuiBuilder()->get_object("pydoc"));
    tb->set_text("");

    Gtk::Dialog* diag;
    VRGuiBuilder()->get_widget("dialog1", diag);
    diag->show();
}

void VRGuiScripts::on_select_help() {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview3"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // get selected object
    VRGuiScripts_HelpColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string obj = row.get_value(cols.obj);
    string type = row.get_value(cols.type);
    string mod = row.get_value(cols.mod);

    VRScriptManager* sm = VRSceneManager::getCurrent();
    Glib::RefPtr<Gtk::TextBuffer> tb  = Glib::RefPtr<Gtk::TextBuffer>::cast_static(VRGuiBuilder()->get_object("pydoc"));

    if (type == "module") {
        vector<string> methods = sm->getPyVRMethods(obj);
        string doc = "\n";
        for (uint i=0; i<methods.size(); i++) {
            string d = sm->getPyVRMethodDoc(mod, methods[i]);
            doc += methods[i] + "\n\t" + d + "\n\n";
        }
        tb->set_text(doc);
    }

    if (type == "method") {
        string doc = "\n" + sm->getPyVRMethodDoc(mod, obj);
        tb->set_text(doc);
    }
}

void VRGuiScripts::on_change_script_type() {
    if(!trigger_cbs) return;
    VRScript* script = getSelectedScript();
    string t = getComboboxText("combobox1");
    script->setType(t);
    on_select_script();
    on_save_clicked();
}

void VRGuiScripts::on_change_mobile() {
    if(!trigger_cbs) return;
    VRScript* script = getSelectedScript();
    script->setHTMLHost( getComboboxText("combobox24") );
    on_select_script();
    on_save_clicked();
}

// --------------------------
// ---------Main-------------
// --------------------------

void VRGuiScripts::printViewerLanguages() {
    GtkSourceLanguageManager* langMgr = gtk_source_language_manager_get_default();
    const gchar* const* ids = gtk_source_language_manager_get_language_ids(langMgr);
    for(const gchar* const* id = ids; *id != NULL; ++id)
        if(ids != NULL) cout << "\nLID " << *id << endl;
}

void VRGuiScripts::updateList() {
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;

    // update script list
    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("script_list"));
    store->clear();

    map<string, VRScript*> fkts = scene->getScripts();
    map<string, VRScript*>::iterator itr;
    for (itr = fkts.begin(); itr != fkts.end(); itr++) {
        Gtk::ListStore::Row row = *store->append();
        gtk_list_store_set (store->gobj(), row.gobj(), 0, itr->first.c_str(), -1);
    }

    on_select_script();
}

void VRGuiScripts::initEditor() {
    // init source view editor
    GtkSourceLanguageManager* langMgr = gtk_source_language_manager_get_default();
    GtkSourceLanguage* lang = gtk_source_language_manager_get_language(langMgr, "python");
    VRGuiScripts_sourceBuffer = gtk_source_buffer_new_with_language(lang);

    Glib::RefPtr<Gtk::ScrolledWindow> win = Glib::RefPtr<Gtk::ScrolledWindow>::cast_static(VRGuiBuilder()->get_object("scrolledwindow4"));
    GtkWidget* editor = gtk_source_view_new_with_buffer(VRGuiScripts_sourceBuffer);
    gtk_container_add (GTK_CONTAINER (win->gobj()), editor);

    // buffer changed callback
    g_signal_connect (VRGuiScripts_sourceBuffer, "changed", G_CALLBACK(VRGuiScripts_on_script_changed), this);

    // editor options
    gtk_source_view_set_tab_width (GTK_SOURCE_VIEW (editor), 4);
    gtk_source_view_set_auto_indent (GTK_SOURCE_VIEW (editor), TRUE);
    gtk_source_view_set_indent_width (GTK_SOURCE_VIEW (editor), 4);
    gtk_source_view_set_highlight_current_line (GTK_SOURCE_VIEW (editor), TRUE);
    gtk_source_view_set_show_line_numbers (GTK_SOURCE_VIEW (editor), TRUE);
    gtk_source_view_set_right_margin_position (GTK_SOURCE_VIEW (editor), 80); // default is 70 chars
    gtk_source_view_set_show_right_margin (GTK_SOURCE_VIEW (editor), TRUE);

    // editor font
    PangoFontDescription *font_desc = pango_font_description_new();
    pango_font_description_set_family (font_desc, "monospace");
    gtk_widget_modify_font (editor, font_desc);
    gtk_widget_show_all(editor);
}

VRGuiScripts::VRGuiScripts() {
    setToolButtonCallback("toolbutton6", sigc::mem_fun(*this, &VRGuiScripts::on_new_clicked) );
    setToolButtonCallback("toolbutton7", sigc::mem_fun(*this, &VRGuiScripts::on_save_clicked) );
    setToolButtonCallback("toolbutton8", sigc::mem_fun(*this, &VRGuiScripts::on_exec_clicked) );
    setToolButtonCallback("toolbutton9", sigc::mem_fun(*this, &VRGuiScripts::on_del_clicked) );
    setToolButtonCallback("toolbutton16", sigc::mem_fun(*this, &VRGuiScripts::on_help_clicked) );

    setButtonCallback("button12", sigc::mem_fun(*this, &VRGuiScripts::on_argadd_clicked) );
    setButtonCallback("button13", sigc::mem_fun(*this, &VRGuiScripts::on_argrem_clicked) );
    setButtonCallback("button23", sigc::mem_fun(*this, &VRGuiScripts::on_trigadd_clicked) );
    setButtonCallback("button24", sigc::mem_fun(*this, &VRGuiScripts::on_trigrem_clicked) );
    setButtonCallback("button16", sigc::mem_fun(*this, &VRGuiScripts::on_help_close_clicked) );

    setComboboxCallback("combobox1", sigc::mem_fun(*this, &VRGuiScripts::on_change_script_type) );
    setComboboxCallback("combobox24", sigc::mem_fun(*this, &VRGuiScripts::on_change_mobile) );

    // trigger tree_view
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview14"));
    tree_view->add_events((Gdk::EventMask)GDK_KEY_PRESS_MASK);
    tree_view->signal_event().connect(sigc::mem_fun(*this, &VRGuiScripts::on_any_event));
    tree_view->signal_key_press_event().connect(sigc::mem_fun(*this, &VRGuiScripts::on_any_key_event));

    setTreeviewSelectCallback("treeview14", sigc::mem_fun(*this, &VRGuiScripts::on_select_trigger) );
    setTreeviewSelectCallback("treeview5", sigc::mem_fun(*this, &VRGuiScripts::on_select_script) );
    setTreeviewSelectCallback("treeview3", sigc::mem_fun(*this, &VRGuiScripts::on_select_help) );

    initEditor();

    setCellRendererCallback("cellrenderertext13", sigc::mem_fun(*this, &VRGuiScripts::on_name_edited) );
    setCellRendererCallback("cellrenderertext2", sigc::mem_fun(*this, &VRGuiScripts::on_argname_edited) );
    setCellRendererCallback("cellrenderertext14", sigc::mem_fun(*this, &VRGuiScripts::on_argval_edited) );
    setCellRendererCallback("cellrenderertext16", sigc::mem_fun(*this, &VRGuiScripts::on_trigparam_edited) );
    setCellRendererCallback("cellrenderertext41", sigc::mem_fun(*this, &VRGuiScripts::on_trigkey_edited) );

    VRGuiScripts_ArgsModelColumns acols;
    VRGuiScripts_TrigsModelColumns tcols;
    setCellRendererCombo("treeviewcolumn16", "arg_types", acols.type, sigc::mem_fun(*this, &VRGuiScripts::on_argtype_edited) );
    setCellRendererCombo("treeviewcolumn27", "ScriptTrigger", tcols.trigs, sigc::mem_fun(*this, &VRGuiScripts::on_trigger_edited) );
    setCellRendererCombo("treeviewcolumn28", "ScriptTriggerDevices", tcols.devs, sigc::mem_fun(*this, &VRGuiScripts::on_trigdev_edited) );
    setCellRendererCombo("treeviewcolumn30", "ScriptTriggerStates", tcols.state, sigc::mem_fun(*this, &VRGuiScripts::on_trigstate_edited) );

    // fill combolists
    const char *arg_types[] = {"int", "float", "str", "VRPyObjectType", "VRPyTransformType", "VRPyGeometryType", "VRPyDeviceType", "VRPySocketType"};
    const char *trigger_types[] = {"none", "on_scene_load", "on_timeout", "on_device", "on_socket"};
    const char *device_types[] = {"mouse", "keyboard", "flystick", "haptic", "mobile"}; // TODO: get from a list in devicemanager or something
    const char *trigger_states[] = {"Pressed", "Released"};
    const char *script_types[] = {"Python", "GLSL", "HTML"};
    fillStringListstore("arg_types", vector<string>(arg_types, end(arg_types)) );
    fillStringListstore("ScriptTrigger", vector<string>(trigger_types, end(trigger_types)) );
    fillStringListstore("ScriptTriggerDevices", vector<string>(device_types, end(device_types)) );
    fillStringListstore("ScriptTriggerStates", vector<string>(trigger_states, end(trigger_states)) );
    fillStringListstore("liststore6", vector<string>(script_types, end(script_types)) );

    setToolButtonSensivity("toolbutton7", false);
    setToolButtonSensivity("toolbutton8", false);
    setToolButtonSensivity("toolbutton9", false);
}

OSG_END_NAMESPACE;
