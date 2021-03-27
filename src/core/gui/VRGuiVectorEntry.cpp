#include <gtk/gtk.h>
#include "VRGuiVectorEntry.h"
#include "core/utils/toString.h"
#include "VRGuiUtils.h"
#include "VRGuiBuilder.h"

VRGuiVectorEntry::VRGuiVectorEntry() {
    ex = ey = ez = 0;
}

bool VRGuiVectorEntry::proxy(GdkEventFocus* focus, function<void(OSG::Vec3d&)> sig, GtkEntry* ex, GtkEntry* ey, GtkEntry* ez) {
    OSG::Vec3d res;
    res[0] = toFloat( gtk_entry_get_text(ex) );
    res[1] = toFloat( gtk_entry_get_text(ey) );
    res[2] = toFloat( gtk_entry_get_text(ez) );
    sig(res);
    return GDK_EVENT_PROPAGATE;
}

bool VRGuiVectorEntry::proxy2D(GdkEventFocus* focus, function<void(OSG::Vec2d&)> sig, GtkEntry* ex, GtkEntry* ey) {
    OSG::Vec2d res;
    res[0] = toFloat( gtk_entry_get_text(ex) );
    res[1] = toFloat( gtk_entry_get_text(ey) );
    sig(res);
    return GDK_EVENT_PROPAGATE;
}

void VRGuiVectorEntry::init(string placeholder, string label,  function<void(OSG::Vec3d&)> sig) {
    auto ph = VRGuiBuilder::get()->get_widget(placeholder.c_str());
    auto frame = gtk_widget_get_parent(ph);
    gtk_container_remove((GtkContainer*)frame, ph);

    auto hb = gtk_hbox_new(false, 0);
    gtk_container_add((GtkContainer*)frame, hb);

    auto lbl = gtk_label_new(label.c_str());
    gtk_widget_set_size_request(lbl, 50, -1);

    ex = (GtkEntry*)gtk_entry_new();
    ey = (GtkEntry*)gtk_entry_new();
    ez = (GtkEntry*)gtk_entry_new();
    gtk_entry_set_has_frame(ex, false);
    gtk_entry_set_has_frame(ey, false);
    gtk_entry_set_has_frame(ez, false);
    gtk_widget_set_size_request((GtkWidget*)ex, 50, -1);
    gtk_widget_set_size_request((GtkWidget*)ey, 50, -1);
    gtk_widget_set_size_request((GtkWidget*)ez, 50, -1);
    gtk_entry_set_width_chars(ex, 5);
    gtk_entry_set_width_chars(ey, 5);
    gtk_entry_set_width_chars(ez, 5);

    auto s1 = gtk_vseparator_new();
    auto s2 = gtk_vseparator_new();
    auto s3 = gtk_vseparator_new();

    gtk_box_pack_start((GtkBox*)hb, lbl, true, true, 2);
    //gtk_box_pack_start((GtkBox*)hb, s1, false, true, 0);
    gtk_box_pack_start((GtkBox*)hb, (GtkWidget*)ex, true, true, 0);
    //gtk_box_pack_start((GtkBox*)hb, s2, false, true, 0);
    gtk_box_pack_start((GtkBox*)hb, (GtkWidget*)ey, true, true, 0);
    //gtk_box_pack_start((GtkBox*)hb, s3, false, true, 0);
    gtk_box_pack_start((GtkBox*)hb, (GtkWidget*)ez, true, true, 0);
    gtk_widget_show_all(frame);

    function<bool(GdkEventFocus*)> sif = bind(&VRGuiVectorEntry::proxy, placeholders::_1, sig, ex, ey, ez);
    connect_signal(ex, sif, "focus_out_event");
    connect_signal(ey, sif, "focus_out_event");
    connect_signal(ez, sif, "focus_out_event");

    function<bool()> sia = bind(&VRGuiVectorEntry::proxy, (GdkEventFocus*)0, sig, ex, ey, ez);
    connect_signal(ex, sia, "activate");
    connect_signal(ey, sia, "activate");
    connect_signal(ez, sia, "activate");
}

void VRGuiVectorEntry::init2D(string placeholder, string label,  function<void(OSG::Vec2d&)> sig) {
    auto ph = VRGuiBuilder::get()->get_widget(placeholder.c_str());
    auto frame = gtk_widget_get_parent(ph);
    gtk_container_remove((GtkContainer*)frame, ph);

    auto hb = gtk_hbox_new(false, 0);
    gtk_container_add((GtkContainer*)frame, hb);

    auto lbl = gtk_label_new(label.c_str());
    gtk_widget_set_size_request(lbl, 50, -1);

    ex = (GtkEntry*)gtk_entry_new();
    ey = (GtkEntry*)gtk_entry_new();
    gtk_entry_set_has_frame(ex, false);
    gtk_entry_set_has_frame(ey, false);
    gtk_widget_set_size_request((GtkWidget*)ex, 50, -1);
    gtk_widget_set_size_request((GtkWidget*)ey, 50, -1);

    auto s1 = gtk_vseparator_new();
    auto s2 = gtk_vseparator_new();

    gtk_box_pack_start((GtkBox*)hb, lbl, true, true, 2);
    //gtk_box_pack_start((GtkBox*)hb, s1, true, true, 0);
    gtk_box_pack_start((GtkBox*)hb, (GtkWidget*)ex, true, true, 0);
    //gtk_box_pack_start((GtkBox*)hb, s2, true, true, 0);
    gtk_box_pack_start((GtkBox*)hb, (GtkWidget*)ey, true, true, 0);
    gtk_widget_show_all(frame);

    function<bool(GdkEventFocus*)> sif = bind(&VRGuiVectorEntry::proxy2D, placeholders::_1, sig, ex, ey);
    connect_signal(ex, sif, "focus_out_event");
    connect_signal(ey, sif, "focus_out_event");

    function<bool()> sia = bind(&VRGuiVectorEntry::proxy2D, (GdkEventFocus*)0, sig, ex, ey);
    connect_signal(ex, sia, "activate");
    connect_signal(ey, sia, "activate");
}


void VRGuiVectorEntry::set(OSG::Vec3d v) {
    gtk_entry_set_text(ex, toString(v[0]).c_str());
    gtk_entry_set_text(ey, toString(v[1]).c_str());
    if (ez) gtk_entry_set_text(ez, toString(v[2]).c_str());
}

void VRGuiVectorEntry::set(OSG::Vec2d v) {
    gtk_entry_set_text(ex, toString(v[0]).c_str());
    gtk_entry_set_text(ey, toString(v[1]).c_str());
}

void VRGuiVectorEntry::setFontColor(OSG::Vec3d c) {
    GdkColor col;
    col.pixel = 0;
    col.red = c[0]*65535;
    col.green = c[1]*65535;
    col.blue = c[2]*65535;
    if (lbl) gtk_widget_modify_fg((GtkWidget*)lbl, gtk_widget_get_state((GtkWidget*)lbl), &col);
    if (ex) gtk_widget_modify_text((GtkWidget*)ex, gtk_widget_get_state((GtkWidget*)ex), &col);
    if (ey) gtk_widget_modify_text((GtkWidget*)ey, gtk_widget_get_state((GtkWidget*)ey), &col);
    if (ez) gtk_widget_modify_text((GtkWidget*)ez, gtk_widget_get_state((GtkWidget*)ez), &col);
}
