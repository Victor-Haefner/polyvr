#ifndef VRGUISCENE_H_INCLUDED
#define VRGUISCENE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <gtkmm/treeview.h>
#include "core/objects/object/VRObject.h"
#include "VRGuiContextMenu.h"
#include "VRGuiVectorEntry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiScene {
    private:
        VRObject* dragDest;
        string dragPath;
        int dragPos = 0;
        VRGuiContextMenu* menu;
        bool cache_override;

        // ------------- transform -----------------------
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
        // ----------------------------------------------

        // ------------- light -----------------------
        void on_toggle_light();
        void on_toggle_light_shadow();
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
        void on_lod_decimate_changed();
        // ----------------------------------------------

        // ------------- scenegraph drag && drop -------
        void on_drag_beg(const Glib::RefPtr<Gdk::DragContext>& dc);
        void on_drag_end(const Glib::RefPtr<Gdk::DragContext>& dc);
        void on_drag_data_receive(const Glib::RefPtr<Gdk::DragContext>& dc , int i1, int i2 ,const Gtk::SelectionData& sd, guint i3, guint i4);
        void on_edit_object_name(string path, string name);
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
        void on_menu_add_csg();
        // ----------------------------------------------


        // ------------- other --------------------------
        void initCallbacks();
        void on_toggle_cache_override();
        void on_collada_import_clicked();
        // ----------------------------------------------

    public:
        VRGuiScene();

        void updateTreeView();
        void update();
};

OSG_END_NAMESPACE;

#endif // VRGUISCENE_H_INCLUDED
