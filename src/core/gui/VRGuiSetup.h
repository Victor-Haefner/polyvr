#ifndef VRGUIDISPLAYS_H_INCLUDED
#define VRGUIDISPLAYS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/utils/VRFunctionFwd.h"
#include "core/setup/VRSetupFwd.h"
#include "core/scene/VRSceneFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "core/scripting/VRScriptFwd.h"
#include "VRGuiVectorEntry.h"
#include "VRGuiEditor.h"

class VRGuiContextMenu;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiSetup {
    private:
        void* selected_object = 0;
        string selected_type;
        string selected_name;

        VRWindowPtr window;
        VRViewPtr view;
        VRNetworkNodePtr node;
        VRNetworkSlavePtr slave;
        VRDevicePtr device;
        ART_devicePtr art_device;
        VRPN_devicePtr vrpn_device;
        VRPN_devicePtr vrpn_tracker;
        VRHapticPtr haptic_device;
        VRLeapPtr leap_device;
        VRMultiTouchPtr mtouch_device;

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

	    VRUpdateCbPtr updatePtr;
	    VRDeviceCbPtr updateSetupCb;

        shared_ptr<VRGuiEditor> editor;

        bool guard; // update guard
        string setupRequest;

        string setupDir();

        void on_treeview_select(string selected);
        void on_treeview_rename(string ID, string name);
        void on_save_clicked();
        void on_save_as_clicked();
        void on_diag_save_as_clicked();
        void on_del_clicked();
        void on_new_clicked();
        void on_foto_clicked(bool b);
        void on_setup_change_request(string name);
        void on_setup_changed();
        bool on_treeview_rightclick();

        void on_menu_add_window();
        void on_menu_add_viewport(string winName);
#ifndef WITHOUT_VRPN
        void on_menu_add_vrpn_tracker();
#endif
        template<class T> void on_menu_add_device();
        void on_menu_add_network_node();
        void on_menu_add_network_slave(string node);
        void on_menu_add_script();
        void on_menu_delete(string node);

        void on_window_mouse_changed(string s);
        void on_window_touch_changed(string s);
        void on_window_kboard_changed(string s);
        void on_window_msaa_changed(string s);
        void on_window_size_changed(int w, int h);
        void on_window_title_changed(string s);
        void on_window_icon_changed(string s);
        void on_window_set_active(bool b);
        void on_toggle_display_multi();
        void on_servern_edit(int Nx, int Ny);
        void on_server_set_connection(string ct);
        void on_server_edit(int x, int y, string sID);
        void on_connect_mw_clicked();

        void on_toggle_view_stats(bool b);
        void on_toggle_display_stereo(bool b);
        void on_toggle_display_projection(bool b);
        void on_toggle_view_invert(bool b);
        void on_toggle_view_active_stereo(bool b);
        void on_toggle_view_user(bool b);
        void on_toggle_view_mirror(bool b);
        void on_change_view_user(string name);
        void on_pos_edit(Vec4d p);
        void on_eyesep_edit(float d);

        void on_displays_edit_offset(Vec3d o);
        void on_displays_set_calib_overlay(bool b);

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
        void on_vrpn_edit_port(int port);
        void on_edit_VRPN_tracker_address(string address);
        void on_toggle_vrpn(bool b);
#endif
#ifndef WITHOUT_ART
        void on_toggle_art(bool b);
        void on_art_edit_port(int port);
        void on_art_edit_axis(Vec3i v);
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

        void on_netnode_address_edited(string s) ;
        void on_netnode_user_edited(string s);
        void on_netnode_path_edited(string s);
        void on_netnode_key_clicked();
        void on_netnode_stopall_clicked();

        void on_netslave_set_autostart(bool b);
        void on_netslave_set_fullscreen(bool b);
        void on_netslave_set_activestereo(bool b);
        void on_netslave_start_clicked();
        void on_netslave_set_port(int port);
        void on_netslave_set_delay(int delay);
        void on_netslave_set_screen(string s);
        void on_netslave_set_geometry(string g);
        void on_netslave_set_connection(string ct);

        void on_script_save_clicked();
        void on_script_exec_clicked();
        void on_script_trigger_switched();

        void on_script_changed();

        void updateObjectData();

        //void setTreeRow(_GtkTreeStore* tree_store, _GtkTreeIter* row, string name, string type, gpointer ptr, string fg = "#000000", string bg = "#FFFFFF");

    public:
        VRGuiSetup();

        VRScriptPtr getSelectedScript();
        shared_ptr<VRGuiEditor> getEditor();

        void updateSetupList();
        bool updateSetup();
        void updateStatus();
};

OSG_END_NAMESPACE

#endif // VRGUIDISPLAYS_H_INCLUDED
