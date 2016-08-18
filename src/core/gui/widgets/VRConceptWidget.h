#ifndef VRCONCEPTWIDGET_H_INCLUDED
#define VRCONCEPTWIDGET_H_INCLUDED

#include "VRSemanticWidget.h"
#include "addons/Semantics/VRSemanticsFwd.h"

OSG_BEGIN_NAMESPACE;

struct VRConceptWidget : public VRSemanticWidget {
    VRConceptPtr concept;
    VRPropertyPtr selected_property;

    VRConceptWidget(VRGuiSemantics* m, Gtk::Fixed* canvas = 0, VRConceptPtr concept = 0);

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

#endif // VRCONCEPTWIDGET_H_INCLUDED
