#ifndef VRSEMANTICWIDGET_H_INCLUDED
#define VRSEMANTICWIDGET_H_INCLUDED

#include <string>
#include "core/math/OSGMathFwd.h"

#include "core/gui/VRGuiFwd.h"
#include "addons/Algorithms/VRGraphLayout.h"

struct _GtkFixed;
struct _GtkFrame;
struct _GtkLabel;
struct _GtkTreeView;
struct _GtkHBox;
struct _GtkToolbar;
struct _GtkToolButton;
struct _GtkTreeIter;

using namespace std;

OSG_BEGIN_NAMESPACE;

class VRGuiSemantics;

struct VRSemanticWidget : public std::enable_shared_from_this<VRSemanticWidget> {
    Vec2d pos;
    bool visible = true;
    bool expanded = false;
    bool subTreeFolded = false;

    VRGuiSemantics* manager = 0;
    _GtkFixed* canvas = 0;
    _GtkFrame* widget = 0;
    _GtkLabel* label = 0;
    _GtkTreeView* treeview = 0;
    _GtkHBox* toolbars = 0;
    _GtkToolbar* toolbar1 = 0;
    _GtkToolButton* bFold = 0;

    map<VRSemanticWidget*, VRSemanticWidgetWeakPtr> children;
    map<VRConnectorWidget*, VRConnectorWidgetWeakPtr> connectors;

    VRSemanticWidget(VRGuiSemantics* m, _GtkFixed* canvas, string color);
    ~VRSemanticWidget();

    VRSemanticWidgetPtr ptr();

    void on_select();
    void on_expander_toggled();

    void move(Vec2d p);
    Vec2d getAnchorPoint(Vec2d p);
    void setPropRow(_GtkTreeIter* iter, string name, string type, string color, int flag, int ID = 0, int rtype = 0);

    Vec3d getPosition();
    Vec3d getSize();

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
