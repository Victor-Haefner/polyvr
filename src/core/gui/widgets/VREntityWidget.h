#ifndef VRENTITYWIDGET_H_INCLUDED
#define VRENTITYWIDGET_H_INCLUDED

#include "VRSemanticWidget.h"
#include "addons/Semantics/VRSemanticsFwd.h"

OSG_BEGIN_NAMESPACE;

struct VREntityWidget : public VRSemanticWidget {
    VREntityPtr entity;
    VRPropertyPtr selected_entity_property;
    VRPropertyPtr selected_concept_property;

    VREntityWidget(VRGuiSemantics* m, VREntityPtr entity = 0);

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

#endif // VRENTITYWIDGET_H_INCLUDED
