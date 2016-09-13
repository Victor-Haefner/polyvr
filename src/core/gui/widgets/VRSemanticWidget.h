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
    class Toolbar;
    class ToolButton;
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

struct VRSemanticWidget : public std::enable_shared_from_this<VRSemanticWidget> {
    Vec2f pos;
    bool visible = true;
    bool expanded = false;
    bool subTreeFolded = false;

    VRGuiSemantics* manager = 0;
    Gtk::Fixed* canvas = 0;
    Gtk::Frame* widget = 0;
    Gtk::Label* label = 0;
    Gtk::TreeView* treeview = 0;
    Gtk::HBox* toolbars = 0;
    Gtk::Toolbar* toolbar1 = 0;
    Gtk::ToolButton* bFold = 0;

    map<VRSemanticWidget*, VRSemanticWidgetWeakPtr> children;
    map<VRConnectorWidget*, VRConnectorWidgetWeakPtr> connectors;

    VRSemanticWidget(VRGuiSemantics* m, Gtk::Fixed* canvas, string color);
    ~VRSemanticWidget();

    VRSemanticWidgetPtr ptr();

    void on_select();
    void on_expander_toggled();

    void move(Vec2f p);
    Vec2f getAnchorPoint(Vec2f p);
    void setPropRow(Gtk::TreeModel::iterator iter, string name, string type, string color, int flag, int ID = 0, int rtype = 0);

    Vec3f getPosition();
    Vec3f getSize();

    void on_fold_clicked();
    void setVisible(bool visible = true);
    void setFolding(bool folded = true);

    virtual void on_select_property() = 0;
    virtual void on_rem_clicked() = 0;
    virtual void on_edit_clicked() = 0;
    virtual void on_newp_clicked() = 0;
    virtual void on_edit_prop_clicked() = 0;
    virtual void on_rem_prop_clicked() = 0;
    virtual int ID() = 0;
    virtual void update() = 0;

    void reparent(VRSemanticWidgetPtr w);
    virtual void reparent(VRConceptWidgetPtr w) = 0;
    virtual void reparent(VREntityWidgetPtr w) = 0;
    virtual void reparent(VRRuleWidgetPtr w) = 0;
};

OSG_END_NAMESPACE;

#endif // VRSEMANTICWIDGET_H_INCLUDED
