#ifndef VRSEMANTICWIDGET_H_INCLUDED
#define VRSEMANTICWIDGET_H_INCLUDED

#include <string>
#include <OpenSG/OSGVector.h>
#include <gtkmm/treemodel.h>

#include "core/gui/VRGuiFwd.h"
#include "addons/Algorithms/VRGraphLayout.h"

using namespace std;

namespace Gtk {
    class Fixed;
    class Frame;
    class Label;
    class TreeView;
    class HBox;
}

class VRGuiSemantics_PropsColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiSemantics_PropsColumns() { add(name); add(type); add(prop); add(ptype); add(flag); add(rtype); add(ID); }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> type;
        Gtk::TreeModelColumn<Glib::ustring> prop;
        Gtk::TreeModelColumn<Glib::ustring> ptype;
        Gtk::TreeModelColumn<int> flag;
        Gtk::TreeModelColumn<int> rtype;
        Gtk::TreeModelColumn<int> ID;
};

OSG_BEGIN_NAMESPACE;

class VRGuiSemantics;

struct VRSemanticWidget {
    Vec2f pos;

    VRGuiSemantics* manager = 0;
    Gtk::Fixed* canvas = 0;
    Gtk::Frame* widget = 0;
    Gtk::Label* label = 0;
    Gtk::TreeView* treeview = 0;
    Gtk::HBox* toolbars = 0;

    VRSemanticWidget(VRGuiSemantics* m, Gtk::Fixed* canvas, string color);
    ~VRSemanticWidget();

    void on_select();
    bool on_expander_clicked(GdkEventButton* e);

    void move(Vec2f p);
    Vec2f getAnchorPoint(Vec2f p);
    void setPropRow(Gtk::TreeModel::iterator iter, string name, string type, string color, int flag, int ID = 0, int rtype = 0);

    VRGraphLayout::Node toGraphLayoutNode();

    virtual void on_select_property() = 0;
    virtual void on_rem_clicked() = 0;
    virtual void on_edit_clicked() = 0;
    virtual void on_newp_clicked() = 0;
    virtual void on_edit_prop_clicked() = 0;
    virtual void on_rem_prop_clicked() = 0;
    virtual int ID() = 0;
    virtual void update() = 0;
};

OSG_END_NAMESPACE;

#endif // VRSEMANTICWIDGET_H_INCLUDED
