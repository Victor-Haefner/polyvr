#ifndef VRSEMANTICWIDGET_H_INCLUDED
#define VRSEMANTICWIDGET_H_INCLUDED

#include <string>
#include "VRCanvasWidget.h"
#include "core/math/OSGMathFwd.h"

using namespace std;

OSG_BEGIN_NAMESPACE;

class VRGuiSemantics;

struct VRSemanticWidget : public VRCanvasWidget {
    bool expanded = false;

    VRGuiSemantics* manager = 0;
    /*_GtkTreeView* treeview;
    _GtkLabel* label = 0;
    _GtkBox* toolbars = 0;
    _GtkToolbar* toolbar1 = 0;
    _GtkToolItem* bFold = 0;*/

    VRSemanticWidget(VRGuiSemantics* m, string color);
    ~VRSemanticWidget();

    VRSemanticWidgetPtr ptr();

    void on_select();
    void on_expander_toggled();
    void on_fold_clicked();

    void setPropRow(string name, string type, string color, int flag, int ID = 0, int rtype = 0);

    virtual void on_select_property() = 0;
    virtual void on_rem_clicked() = 0;
    virtual void on_edit_clicked() = 0;
    virtual void on_newp_clicked() = 0;
    virtual void on_edit_prop_clicked() = 0;
    virtual void on_rem_prop_clicked() = 0;
    virtual void update() = 0;

    void reparent(VRSemanticWidgetPtr w);
    virtual void reparent(VRConceptWidgetPtr w) = 0;
    virtual void reparent(VREntityWidgetPtr w) = 0;
    virtual void reparent(VRRuleWidgetPtr w) = 0;
};

OSG_END_NAMESPACE;

#endif // VRSEMANTICWIDGET_H_INCLUDED
