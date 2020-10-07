#include "VRGuiWidget.h"
#include "../VRGuiUtils.h"
#include "../VRGuiBuilder.h"

#include <gtk/gtk.h>

VRGuiWidget::VRGuiWidget(string name) {
    widget = VRGuiBuilder::get()->get_widget(name);
}

VRGuiWidget::VRGuiWidget(_GtkWidget* w) {
    widget = w;
}

VRGuiWidget::~VRGuiWidget() {}

void VRGuiWidget::setVisibility(bool b, bool all) {
    if (!all) {
        if (b) gtk_widget_show(widget);
        else   gtk_widget_hide(widget);
    } else {
        if (b) gtk_widget_show_all(widget);
#if GTK_MAJOR_VERSION == 2
        else   gtk_widget_hide_all(widget);
#else
        else   gtk_widget_hide(widget);
#endif
    }
}

void VRGuiWidget::setSensitivity(bool b) {
    gtk_widget_set_sensitive(widget, b);
}

void VRGuiWidget::grabFocus() {
    gtk_widget_grab_focus(widget);
}

