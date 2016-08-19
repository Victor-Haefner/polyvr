#ifndef VRRULEWIDGET_H_INCLUDED
#define VRRULEWIDGET_H_INCLUDED

#include "VRSemanticWidget.h"
#include "addons/Semantics/VRSemanticsFwd.h"

OSG_BEGIN_NAMESPACE;

struct VRRuleWidget : public VRSemanticWidget {
    VROntologyRulePtr rule;
    VRStatementPtr selected_statement;

    VRRuleWidget(VRGuiSemantics* m, Gtk::Fixed* canvas = 0, VROntologyRulePtr rule = 0);

    void on_new_clicked();
    void on_select_property();
    void on_rem_clicked();
    void on_edit_clicked();
    void on_newp_clicked();
    void on_edit_prop_clicked();
    void on_rem_prop_clicked();
    void update();
};

OSG_END_NAMESPACE;

#endif // VRRULEWIDGET_H_INCLUDED
