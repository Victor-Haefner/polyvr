#ifndef VRGUISCENE_H_INCLUDED
#define VRGUISCENE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include "core/objects/object/VRObject.h"
#include "VRGuiContextMenu.h"
#include "VRGuiVectorEntry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiScene {
    private:
        VRObjectWeakPtr dragDest;
        string dragPath;
        int dragPos = 0;
        bool transformModeLocal = true;
        VRGuiContextMenu* menu;

        Glib::RefPtr<Gtk::TreeStore> tree_store;
        Glib::RefPtr<Gtk::TreeView> tree_view;

        Gtk::TreeModel::iterator selected_itr;
        int selected = -1;
        VRGeometryWeakPtr selected_geometry;
        VRObjectWeakPtr VRGuiScene_copied;
        bool liveUpdate = false;
        bool trigger_cbs = false;

        VRGuiVectorEntry posEntry;
        VRGuiVectorEntry atEntry;
        VRGuiVectorEntry dirEntry;
        VRGuiVectorEntry upEntry;
        VRGuiVectorEntry scaleEntry;
        VRGuiVectorEntry ctEntry;
        VRGuiVectorEntry lodCEntry;

        // ---------ObjectForms------
        VRObjectPtr getSelected();
        void updateObjectForms(bool disable = false);
        void setObject(VRObjectPtr o);
        void setTransform(VRTransformPtr e);
        void setMaterial(VRMaterialPtr mat);
        void setGeometry(VRGeometryPtr g);
        void setLight(VRLightPtr l);
        void setCamera(VRCameraPtr c);
        void setGroup(VRGroupPtr g);
        void setLod(VRLodPtr lod);
        void setEntity(VREntityPtr e);
        // --------------------------

        // ---------TreeView---------
        void on_treeview_select();
        void getTypeColors(VRObjectPtr o, string& fg, string& bg);
        void setSGRow(Gtk::TreeModel::iterator itr, VRObjectPtr o);
        void parseSGTree(VRObjectPtr o, Gtk::TreeModel::iterator itr = Gtk::TreeModel::iterator());
        void removeTreeStoreBranch(Gtk::TreeModel::iterator iter, bool self = true);
        void syncSGTree(VRObjectPtr o, Gtk::TreeModel::iterator itr);
        // ----------------------------------------------

        // ------------- transform -----------------------
        void on_change_from(Vec3d v);
        void on_change_at(Vec3d v);
        void on_change_dir(Vec3d v);
        void on_change_up(Vec3d v);
        void on_scale_changed(Vec3d v);
        void on_change_lod_center(Vec3d v);
        void on_edit_T_constraint(Vec3d v);
        void on_toggle_T_mode();
        void on_toggle_T_constraint_mode();
        void on_toggle_phys();
        void on_toggle_dynamic();
        void on_change_phys_shape();
        void on_mass_changed();
        // ----------------------------------------------

        // ------------- geometry -----------------------
        void setGeometry_gui();
        void setGeometry_material();
        void setGeometry_shadow_toggle();
        void setGeometry_shadow_type();
        void setGeometry_type();
        void setGeometry_file();
        void setGeometry_object();
        void setGeometry_vertex_count();
        void setGeometry_face_count();
        void on_edit_primitive_params(string path, string new_text);
        // ----------------------------------------------

        // ------------- light -----------------------
        void on_toggle_light();
        void on_toggle_light_shadow();
        void on_toggle_light_shadow_volume();
        void on_change_light_type();
        void on_change_light_shadow();
        void on_edit_light_attenuation();
        bool setShadow_color(GdkEventButton* b);
        bool setLight_diff_color(GdkEventButton* b);
        bool setLight_amb_color(GdkEventButton* b);
        bool setLight_spec_color(GdkEventButton* b);
        // ----------------------------------------------

        // ------------- camera -----------------------
        void on_cam_aspect_changed();
        void on_cam_fov_changed();
        void on_cam_near_changed();
        void on_cam_far_changed();
        void on_change_cam_proj();
        // ----------------------------------------------

        // ------------- material -----------------------
        void setMaterial_gui();
        void setMaterial_lit();
        bool setMaterial_diffuse(GdkEventButton* b);
        bool setMaterial_specular(GdkEventButton* b);
        bool setMaterial_ambient(GdkEventButton* b);
        void setMaterial_pointsize();
        void setMaterial_texture_toggle();
        void setMaterial_texture_name();
        // ----------------------------------------------

        // ------------- lod -----------------------
        void on_edit_distance(string path, string new_text);
        void on_lod_decimate_changed();
        // ----------------------------------------------

        // ------------- scenegraph drag && drop -------
        void on_drag_beg(const Glib::RefPtr<Gdk::DragContext>& dc);
        void on_drag_end(const Glib::RefPtr<Gdk::DragContext>& dc);
        void on_drag_data_receive(const Glib::RefPtr<Gdk::DragContext>& dc , int i1, int i2 ,const Gtk::SelectionData& sd, guint i3, guint i4);
        void on_edit_object_name(string path, string new_text);
        // ----------------------------------------------

        // ------------- context menu -------------------
        void initMenu();
        bool on_treeview_rightclick(GdkEventButton * event);
        void on_menu_delete();
        void on_menu_copy();
        void on_menu_paste();
        template<class T>
        void on_menu_add();
        void on_menu_add_file();
        void on_menu_add_light();
        void on_menu_add_camera();
        void on_menu_add_animation();
        void on_menu_add_primitive(string s);
        //void on_menu_add_csg();
        // ----------------------------------------------


        // ------------- other --------------------------
        void initCallbacks();
        void on_collada_import_clicked();

        void on_focus_clicked();
        void on_identity_clicked();
        void on_groupsync_clicked();
        void on_groupapply_clicked();
        void on_scene_update();

        void on_group_edited();
        void on_change_primitive();
        void on_change_group();

        void on_toggle_liveupdate();
        void on_toggle_visible();
        void on_toggle_pickable();
        void on_toggle_rc();
        void on_toggle_T_constraint();
        void on_toggle_R_constraint();
        void on_toggle_camera_accept_realroot();
        // ----------------------------------------------

    public:
        VRGuiScene();

        void updateTreeView();
        void update();
};

OSG_END_NAMESPACE;

#endif // VRGUISCENE_H_INCLUDED
