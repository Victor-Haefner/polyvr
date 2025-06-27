#ifndef VRGUISCENE_H_INCLUDED
#define VRGUISCENE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/object/VRObject.h"
#include "core/objects/material/VRMaterialFwd.h"
#include "VRGuiVectorEntry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiScene {
    private:
        VRObjectWeakPtr dragDest;
        VRObjectWeakPtr dragObj;
        string dragPath;
        int dragPos = 0;
        bool transformModeLocal = true;

        int selected = -1;
        VRGeometryWeakPtr selected_geometry;
        VRObjectWeakPtr VRGuiScene_copied;
        bool liveUpdate = false;
        bool trigger_cbs = false;

        string toBeDeleted;

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
        void on_treeview_select(string selected);
        void on_treeview_rename(string ID, string name);
        void getTypeColors(VRObjectPtr o, string& fg, string& bg);
        void setSGRow(VRObjectPtr o);
        void parseSGTree(VRObjectPtr o, string parent = "");
        void removeTreeStoreBranch(bool self = true);
        void syncSGTree(VRObjectPtr o);
        // ----------------------------------------------

        // ------------- transform -----------------------
        void on_change_from(Vec3d v);
        void on_change_at(Vec3d v);
        void on_change_dir(Vec3d v);
        void on_change_up(Vec3d v);
        void on_scale_changed(Vec3d v);
        void on_toggle_global(bool global);

        void on_constraint_set_active(bool b);
        void on_constraint_set_local(bool b);
        void on_constraint_lock_rotation(bool b);
        void on_constraint_set_dof(int dof, double min, double max);

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
        void on_edit_primitive_params(const char* path_string, const char* new_text);
        // ----------------------------------------------

        // ------------- light -----------------------
        void on_toggle_light(bool b);
        void on_toggle_light_shadow(bool b);
        void on_toggle_light_shadow_volume(bool b, float D);
        void on_change_light_type(string type);
        void on_change_light_shadow(int res);
        void on_edit_light_attenuation(Vec3d a);
        bool setShadow_color(Color4f c);
        bool setLight_diff_color(Color4f c);
        bool setLight_amb_color(Color4f c);
        bool setLight_spec_color(Color4f c);
        // ----------------------------------------------

        // ------------- camera -----------------------
        void on_cam_aspect_changed(float v);
        void on_cam_fov_changed(float v);
        void on_cam_near_changed(float v);
        void on_cam_far_changed(float v);
        void on_change_cam_proj(string mode);
        // ----------------------------------------------

        // ------------- material -----------------------
        void setMaterial_gui();
        void setMaterial_lit(bool b);
        void setMaterial_meshcolors(bool b);
        bool setMaterial_diffuse(Color4f c);
        bool setMaterial_specular(Color4f c);
        bool setMaterial_ambient(Color4f c);
        bool setMaterial_emission(Color4f c);
        void setMaterial_pointsize(int ps);
        void setMaterial_linewidth(int lw);
        void setMaterial_texture_toggle();
        void setMaterial_texture_name();
        // ----------------------------------------------

        // ------------- lod -----------------------
        void on_edit_distance(const char* path_string, const char* new_text);
        void on_lod_decimate_changed();
        void on_change_lod_center(Vec3d v);
        // ----------------------------------------------

        // ------------- scenegraph drag && drop -------
        void on_treeview_drop(string sID, string tID);
        void on_edit_object_name(const char* path_string, const char* new_text);
        // ----------------------------------------------

        // ------------- context menu -------------------
        void initMenu();
        bool on_treeview_rightclick();
        void on_menu_delete(string node);
        void on_menu_copy(string node);
        void on_menu_paste(string node);
        template<class T>
        void on_menu_add(string node);
        void on_menu_add_file(string node);
        void on_menu_add_light(string node);
        void on_menu_add_camera(string node);
        void on_menu_add_animation(string node);

        void on_geo_menu_print();
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
        void on_toggle_visible(bool b);
        void on_toggle_pickable(bool b);
        void on_toggle_throw_shadow(bool b);
        void on_toggle_rc();
        void on_toggle_T_constraint();
        void on_toggle_R_constraint();
        void on_toggle_camera_accept_realroot(bool b);
        // ----------------------------------------------

    public:
        VRGuiScene();

        bool updateTreeView();
        void update();

        void selectObject(VRObjectPtr obj);
};

OSG_END_NAMESPACE;

#endif // VRGUISCENE_H_INCLUDED
