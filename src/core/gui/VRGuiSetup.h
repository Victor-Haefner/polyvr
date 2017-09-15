#ifndef VRGUIDISPLAYS_H_INCLUDED
#define VRGUIDISPLAYS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/utils/VRFunctionFwd.h"
#include "core/setup/VRSetupFwd.h"
#include "core/scene/VRSceneFwd.h"
#include "VRGuiVectorEntry.h"
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>

class VRGuiContextMenu;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRMultiWindow;

class VRGuiSetup {
    private:
        Gtk::TreeModel::Row selected_row;
        Gtk::TreeModel::Row parent_row;
        gpointer selected_object;
        gpointer selected_object_parent;
        string selected_type;
        string selected_name;
        VRSetupWeakPtr current_setup;
        VRSceneWeakPtr current_scene;

        VRGuiVectorEntry centerEntry;
        VRGuiVectorEntry userEntry;
        VRGuiVectorEntry normalEntry;
        VRGuiVectorEntry upEntry;
        VRGuiVectorEntry sizeEntry;
        VRGuiVectorEntry shearEntry;
        VRGuiVectorEntry warpEntry;
        VRGuiVectorEntry vsizeEntry;
        VRGuiVectorEntry mirrorPosEntry;
        VRGuiVectorEntry mirrorNormEntry;

        VRGuiVectorEntry tVRPNAxisEntry;
        VRGuiVectorEntry rVRPNAxisEntry;

        VRGuiContextMenu* menu;
        VRMultiWindow* mwindow;
	    VRUpdateCbPtr updatePtr;
	    VRDeviceCbPtr updateSetupCb;

        bool guard; // update guard

        string setupDir();

        void on_treeview_select();
        void on_name_edited(const Glib::ustring& path, const Glib::ustring& new_name);
        void on_save_clicked();
        void on_del_clicked();
        void on_new_clicked();
        void on_foto_clicked();
        void on_setup_changed();
        bool on_treeview_rightclick(GdkEventButton* event);

        void on_menu_add_window();
        void on_menu_add_viewport();
        void on_menu_add_vrpn_tracker();
        template<class T> void on_menu_add_device();
        void on_menu_add_network_node();
        void on_menu_add_network_slave();
        void on_menu_delete();

        void on_toggle_display_active();
        void on_toggle_display_multi();
        void on_servern_edit();
        void on_server_ct_toggled();
        void on_server_edit(const Glib::ustring& path, const Glib::ustring& new_name);
        void on_connect_mw_clicked();

        void on_toggle_view_stats();
        void on_toggle_display_stereo();
        void on_toggle_display_projection();
        void on_toggle_view_invert();
        void on_toggle_view_active_stereo();
        void on_toggle_view_user();
        void on_toggle_view_mirror();
        void on_change_view_user();
        void on_pos_edit();
        void on_eyesep_edit();

        void on_displays_edit_offset();
        void on_view_size_edit(Vec2d v);
        void on_proj_user_edit(Vec3d v);
        void on_proj_center_edit(Vec3d v);
        void on_proj_normal_edit(Vec3d v);
        void on_proj_up_edit(Vec3d v);
        void on_proj_size_edit(Vec2d v);
        void on_proj_shear_edit(Vec2d v);
        void on_proj_warp_edit(Vec2d v);
        void on_view_mirror_pos_edit(Vec3d v);
        void on_view_mirror_norm_edit(Vec3d v);

        void on_vrpn_edit_port();
        void on_edit_VRPN_tracker_address();
        void on_toggle_vrpn();
        void on_toggle_art();
        void on_art_edit_port();
        void on_art_edit_id();
        void on_art_edit_offset();

        void on_toggle_vrpn_verbose();
        void on_toggle_vrpn_test_server();
        void on_vrpn_trans_axis_edit(Vec3d v);
        void on_vrpn_rot_axis_edit(Vec3d v);

        void on_haptic_ip_edited();
        void on_change_haptic_type();
        void on_toggle_dev_cross();

        void on_netnode_edited();
        void on_netnode_key_clicked();
        void on_netnode_stopall_clicked();
        void on_netslave_edited();
        void on_netslave_start_clicked();

        void closeAllExpander();
        void updateObjectData();

        void setTreeRow(Glib::RefPtr<Gtk::TreeStore> tree_store, Gtk::TreeStore::Row row, string name, string type, gpointer ptr, string fg = "#000000", string bg = "#FFFFFF");

    public:
        VRGuiSetup();

        void updateSetupList();
        void updateSetup();
        void updateStatus();
};

OSG_END_NAMESPACE

#endif // VRGUIDISPLAYS_H_INCLUDED
