#ifndef VRGUISCRIPTS_H_INCLUDED
#define VRGUISCRIPTS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <gtkmm/treemodel.h>
#include <gdkmm/event.h>
#include <gtkmm/liststore.h>
#include "core/scene/VRSceneManager.h"
#include "VRGuiSignals.h"

struct _GtkSourceLanguage;
namespace Gtk{ class Table; class TreeView; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScript;

class VRGuiScripts {
    private:
        struct page {
            int line = 0;
        };

        bool trigger_cbs = true;
        _GtkSourceLanguage* python = 0;
        _GtkSourceLanguage* web = 0;
        _GtkSourceLanguage* glsl = 0;
        map<VRScript*, page> pages;

        Gtk::Table* scriptImportWidget = 0;
        Gtk::TreeView* import_treeview1 = 0;
        Gtk::TreeView* import_treeview2 = 0;
        Glib::RefPtr<Gtk::ListStore> import_liststore1;
        Glib::RefPtr<Gtk::ListStore> import_liststore2;
        map<string, VRScript*> import_scripts;

        void initEditor();
        void printViewerLanguages();
        void setScriptListRow(Gtk::TreeIter itr, VRScript* script, bool onlyTime = false);

        void on_new_clicked();
        void on_save_clicked();
        void on_exec_clicked();
        void on_del_clicked();
        void on_import_clicked();

        void on_select_script();
        void on_change_script_type();
        void on_change_mobile();
        void on_name_edited(const Glib::ustring& path, const Glib::ustring& new_name);
        void on_script_changed();
        bool on_any_key_event(GdkEventKey*);
        bool on_any_event(GdkEvent*);

        void on_select_trigger();
        void on_trigger_edited(const Glib::ustring& new_name, const Gtk::TreeModel::iterator& new_iter);
        void on_trigdev_edited(const Glib::ustring& new_name, const Gtk::TreeModel::iterator& new_iter);
        void on_trigparam_edited(const Glib::ustring& path, const Glib::ustring& new_name);
        void on_trigkey_edited(const Glib::ustring& path, const Glib::ustring& new_name);
        void on_trigstate_edited(const Glib::ustring& new_name, const Gtk::TreeModel::iterator& new_iter);
        void on_trigadd_clicked();
        void on_trigrem_clicked();

        void loadHelp();
        void on_select_help();
        void on_help_clicked();
        void on_help_close_clicked();

        void on_argadd_clicked();
        void on_argrem_clicked();
        void on_argname_edited(const Glib::ustring& path, const Glib::ustring& new_name);
        void on_argval_edited(const Glib::ustring& path, const Glib::ustring& new_name);
        void on_argtype_edited(const Glib::ustring& new_name, const Gtk::TreeModel::iterator& new_iter);

        void on_diag_import_select_1();
        void on_diag_import_select_2();
        void on_diag_import_select();
        void on_diag_import();

        void on_find_clicked();
        void on_find_diag_cancel_clicked();
        void on_find_diag_find_clicked();
        void on_toggle_find_replace();

    public:
        VRGuiScripts();

        void updateList();
        VRScript* getSelectedScript();
        string get_editor_core(int i);

        void update();
};

OSG_END_NAMESPACE

#endif // VRGUISCRIPTS_H_INCLUDED
