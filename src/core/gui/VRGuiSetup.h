#ifndef VRGUIDISPLAYS_H_INCLUDED
#define VRGUIDISPLAYS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/utils/VRFunctionFwd.h"
#include "core/setup/VRSetupFwd.h"
#include "core/scene/VRSceneFwd.h"
#include "core/scripting/VRScriptFwd.h"
#include "VRGuiVectorEntry.h"
#include "VRGuiEditor.h"

struct _GtkTreeIter;
struct _GtkTreeStore;
struct _GdkEventButton;
typedef void* gpointer;

class VRGuiContextMenu;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiSetup {
    private:
        _GtkTreeIter* parent_row = 0;
        gpointer selected_object;
        gpointer selected_object_parent;
        string selected_type;
        string selected_name;
        VRSetupWeakPtr current_setup;
        VRSceneWeakPtr current_scene;

        VRGuiVectorEntry artAxis;
        VRGuiVectorEntry artOffset;
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

        VRGuiVectorEntry leapPosEntry;
        VRGuiVectorEntry leapDirEntry;
        VRGuiVectorEntry leapUpEntry;

        VRGuiContextMenu* menu = 0;
        VRWindow* window = 0;
        VRMultiWindow* mwindow = 0;
	    VRUpdateCbPtr updatePtr;
	    VRDeviceCbPtr updateSetupCb;

        shared_ptr<VRGuiEditor> editor;

        bool guard; // update guard

        string setupDir();

        void on_treeview_select();
        void on_name_edited(const char* path, const char* new_name);
        void on_save_clicked();
        void on_save_as_clicked();
        void on_diag_save_as_clicked();
        void on_del_clicked();
        void on_new_clicked();
        void on_foto_clicked();
        void on_setup_changed();
        bool on_treeview_rightclick(_GdkEventButton* event);

        void on_menu_add_window();
        void on_menu_add_viewport();
#ifndef WITHOUT_VRPN
        void on_menu_add_vrpn_tracker();
#endif
        template<class T> void on_menu_add_device();
        void on_menu_add_network_node();
        void on_menu_add_network_slave();
        void on_menu_add_script();
        void on_menu_delete();

        void on_window_device_changed();
        void on_window_msaa_changed();
        void on_toggle_display_active();
        void on_toggle_display_multi();
        void on_servern_edit();
        void on_server_ct_toggled();
        void on_server_edit(const char* path, const char* new_name);
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

#ifndef WITHOUT_VRPN
        void on_vrpn_edit_port();
        void on_edit_VRPN_tracker_address();
        void on_toggle_vrpn();
#endif
#ifndef WITHOUT_ART
        void on_toggle_art();
        void on_art_edit_port();
        void on_art_edit_id();
        void on_art_edit_axis(Vec3d v);
        void on_art_edit_offset(Vec3d v);
#endif

#ifndef WITHOUT_ART
        void on_toggle_vrpn_verbose();
        void on_toggle_vrpn_test_server();
        void on_vrpn_trans_axis_edit(Vec3d v);
        void on_vrpn_rot_axis_edit(Vec3d v);
#endif

#ifndef WITHOUT_MTOUCH
        void on_mt_device_changed();
#endif
        void on_leap_host_edited();
        void on_leap_startcalib_clicked();
        void on_leap_stopcalib_clicked();
        void on_leap_pos_edit(Vec3d v);
        void on_leap_up_edit(Vec3d v);
        void on_leap_dir_edit(Vec3d v);
#ifndef WITHOUT_VIRTUOSE
        void on_haptic_ip_edited();
        void on_change_haptic_type();
#endif
        void on_toggle_dev_cross();

        void on_netnode_edited();
        void on_netnode_key_clicked();
        void on_netnode_stopall_clicked();
        void on_netslave_edited();
        void on_netslave_start_clicked();

        void on_script_save_clicked();
        void on_script_exec_clicked();
        void on_script_trigger_switched();

        void on_script_changed();

        void closeAllExpander();
        void updateObjectData();

        void setTreeRow(_GtkTreeStore* tree_store, _GtkTreeIter* row, string name, string type, gpointer ptr, string fg = "#000000", string bg = "#FFFFFF");

    public:
        VRGuiSetup();

        VRScriptPtr getSelectedScript();
        shared_ptr<VRGuiEditor> getEditor();

        void updateSetupList();
        void updateSetup();
        void updateStatus();
};

OSG_END_NAMESPACE

#endif // VRGUIDISPLAYS_H_INCLUDED
