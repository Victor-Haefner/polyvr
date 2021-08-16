#ifndef VRRULEWIDGET_H_INCLUDED
#define VRRULEWIDGET_H_INCLUDED

#include "VRSemanticWidget.h"
#include "addons/Semantics/VRSemanticsFwd.h"

OSG_BEGIN_NAMESPACE;

struct VRRuleWidget : public VRSemanticWidget {
    VROntologyRulePtr rule;
    VRStatementPtr selected_statement;

    VRRuleWidget(VRGuiSemantics* m, _GtkFixed* canvas = 0, VROntologyRulePtr rule = 0);

    void on_new_clicked();
    void on_select_property() override;
    void on_rem_clicked() override;
    void on_edit_clicked() override;
    void on_newp_clicked() override;
    void on_edit_prop_clicked() override;
    void on_rem_prop_clicked() override;
    int ID() override;
    void update() override;

    void reparent(VRConceptWidgetPtr w) override;
    void reparent(VREntityWidgetPtr w) override;
    void reparent(VRRuleWidgetPtr w) override;
};

OSG_END_NAMESPACE;

#endif // VRRULEWIDGET_H_INCLUDED
