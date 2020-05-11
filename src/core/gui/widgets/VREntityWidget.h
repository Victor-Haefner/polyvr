#ifndef VRENTITYWIDGET_H_INCLUDED
#define VRENTITYWIDGET_H_INCLUDED

#include "VRSemanticWidget.h"
#include "addons/Semantics/VRSemanticsFwd.h"

OSG_BEGIN_NAMESPACE;

struct VREntityWidget : public VRSemanticWidget {
    VREntityPtr entity;
    VRPropertyPtr selected_entity_property;
    VRPropertyPtr selected_concept_property;

    VREntityWidget(VRGuiSemantics* m, _GtkFixed* canvas = 0, VREntityPtr entity = 0);

    void on_new_clicked();
    void on_select_property();
    void on_rem_clicked();
    void on_edit_clicked();
    void on_newp_clicked();
    void on_edit_prop_clicked();
    void on_rem_prop_clicked();
    int ID();
    void update();

    void reparent(VRConceptWidgetPtr w);
    void reparent(VREntityWidgetPtr w);
    void reparent(VRRuleWidgetPtr w);
};

OSG_END_NAMESPACE;

#endif // VRENTITYWIDGET_H_INCLUDED
