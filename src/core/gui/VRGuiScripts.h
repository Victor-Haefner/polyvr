#ifndef VRGUISCRIPTS_H_INCLUDED
#define VRGUISCRIPTS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/scene/VRSceneManager.h"
#include "VRGuiEditor.h"
#include "VRGuiSignals.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/scripting/VRScriptFwd.h"

struct _GtkSourceLanguage;
struct _GtkTable;
struct _GtkTreeView;
struct _GtkListStore;
struct _GdkEventKey;
struct _GtkTreeIter;
union _GdkEvent;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScript;

class VRGuiScripts {
    private:
        struct group {
            string name = "new Group";
            int ID;
            vector<VRScriptWeakPtr> scripts;
            group();
        };

        struct searchResult {
            string scriptName;
            int line;
            int column;
            searchResult(string s, int l, int c);
        };

        struct pagePos {
            int line = 0;
            int column = 0;
        };

        shared_ptr<VRGuiEditor> editor;
        map<string, pagePos> pages;
        bool trigger_cbs = true;
        bool doPerf = false;
	    VRUpdateCbPtr updatePtr;
	    VRDeviceCbPtr sceneChangedCb;
	    string selected;

        _GtkTable* scriptImportWidget = 0;
        _GtkTreeView* import_treeview1 = 0;
        _GtkTreeView* import_treeview2 = 0;
        _GtkListStore* import_liststore1;
        _GtkListStore* import_liststore2;
        map<string, VRScriptPtr> import_scripts;
        vector<pair<VRScriptWeakPtr, _GtkTreeIter>> scriptRows;

        string docs_filter;
        string templ_filter;
        map<int, group> groups;

        void initEditor();
        void printViewerLanguages();
        void setGroupListRow(_GtkTreeIter* itr, group& g);
        void setScriptListRow(_GtkTreeIter* itr, VRScriptPtr script, bool onlyTime = false);

        void on_new_clicked();
        void on_template_clicked();
        void on_addSep_clicked();
        void on_save_clicked();
        void on_exec_clicked();
        void on_del_clicked();
        void on_import_clicked();
        void on_perf_toggled();
        void on_pause_toggled();

        void on_select_script();
        void on_buffer_changed();
        void on_focus_out_changed(_GdkEvent*);
        void on_change_script_type();
        void on_change_group();
        void on_change_server();
        void on_name_edited(const char* path, const char* new_name);
        void on_script_changed();
        bool on_any_key_event(_GdkEventKey*);
        void on_any_event(_GdkEvent*);

        void on_select_trigger();
        void on_trigger_edited(const char* new_name, _GtkTreeIter* new_iter);
        void on_trigdev_edited(const char* new_name, _GtkTreeIter* new_iter);
        void on_trigparam_edited(const char* path, const char* new_name);
        void on_trigkey_edited(const char* path, const char* new_name);
        void on_trigstate_edited(const char* new_name, _GtkTreeIter* new_iter);
        void on_trigadd_clicked();
        void on_trigrem_clicked();

        void updateDocumentation();
        void on_select_help();
        void on_help_clicked();
        void on_help_close_clicked();
        void on_doc_filter_edited();
        bool on_help_close_frame_clicked(_GdkEvent* event);

        void updateTemplates();
        void on_select_templ();
        void on_templ_close_clicked();
        void on_templ_import_clicked();
        void on_templ_filter_edited();

        void on_argadd_clicked();
        void on_argrem_clicked();
        void on_argname_edited(const char* path, const char* new_name);
        void on_argval_edited(const char* path, const char* new_name);
        void on_argtype_edited(const char* new_name, _GtkTreeIter* new_iter);

        void on_diag_import_select_1();
        void on_diag_import_select_2();
        void on_diag_import_select();
        void on_diag_import();

        void on_find_clicked();
        void on_find_diag_cancel_clicked();
        void on_find_diag_find_clicked();
        void on_toggle_find_replace();
        void on_search_link_clicked(searchResult res, string s);

        void on_scene_changed();

    public:
        VRGuiScripts();

        void updateList();
        VRScriptPtr getSelectedScript();
        void focusScript(string name, int line, int column);
        void getLineFocus(int& line, int& column);
        void update();
        shared_ptr<VRGuiEditor> getEditor();
};

OSG_END_NAMESPACE

#endif // VRGUISCRIPTS_H_INCLUDED
