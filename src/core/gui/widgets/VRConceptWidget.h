#ifndef VRCONCEPTWIDGET_H_INCLUDED
#define VRCONCEPTWIDGET_H_INCLUDED

#include "VRSemanticWidget.h"
#include "addons/Semantics/VRSemanticsFwd.h"

OSG_BEGIN_NAMESPACE;

struct VRConceptWidget : public VRSemanticWidget {
    VRConceptPtr concept;
    VRPropertyPtr selected_property;

    VRConceptWidget(VRGuiSemantics* m, VRConceptPtr concept = 0);

    void on_new_concept_clicked();
    void on_new_entity_clicked();
    void on_new_rule_clicked();

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

#endif // VRCONCEPTWIDGET_H_INCLUDED
