#include <gtk/gtk.h>
#include "VRGuiContextMenu.h"
#include "VRGuiUtils.h"

VRGuiContextMenu::VRGuiContextMenu(string name) {
    ;
}

bool signal_activated_(GdkEventButton* event, function<void()> fkt) {
    fkt();
    return true;
}

void on_item_select(GtkMenu* m) { cout << "\nMenu selected: " << m << endl; }

void VRGuiContextMenu::appendItem( string menu, string item, function<void()> fkt ) {
    if (menus.count(menu) == 0) menus[menu] = (GtkMenu*)gtk_menu_new();
    GtkMenu* m = menus[menu];
    GtkMenuItem* mi = (GtkMenuItem*)gtk_menu_item_new_with_label(item.c_str());

    connect_signal<void, GdkEventButton*>(mi, bind(signal_activated_, placeholders::_1, fkt), "button_press_event");
    gtk_container_add((GtkContainer*)m, (GtkWidget*)mi);
    gtk_widget_show_all((GtkWidget*)m);
}

void VRGuiContextMenu::appendMenu( string menu, string item, string new_menu ) {
    if (menus.count(menu) == 0) menus[menu] = (GtkMenu*)gtk_menu_new();
    GtkMenuItem* mi = (GtkMenuItem*)gtk_menu_item_new_with_mnemonic(item.c_str());
    if (menus.count(new_menu) == 0) menus[new_menu] = (GtkMenu*)gtk_menu_new();
    gtk_menu_item_set_submenu(mi, (GtkWidget*)menus[new_menu]);

    gtk_container_add((GtkContainer*)menus[menu], (GtkWidget*)mi);
    gtk_widget_show_all((GtkWidget*)menus[menu]);
}

void VRGuiContextMenu::popup(string menu, GdkEventButton* event) {
    if (menus.count(menu) == 0) { cout << "\nVRGuiContextMenu::popup, no such menu: " << menu << endl; return; }
    gtk_menu_popup(menus[menu], NULL, NULL, NULL, NULL, event->button, event->time);
}

bool VRGuiContextMenu::on_widget_rightclick(GdkEventButton * event, string menu) {
    if (event->type != GDK_BUTTON_RELEASE) return false;
    if (event->button != 3) return false;
    popup(menu, event); return true;
}

void VRGuiContextMenu::connectWidget(string menu, GtkWidget* widget) {
    gtk_widget_add_events(widget, (int)GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(widget, (int)GDK_BUTTON_RELEASE_MASK);

    function<void(GdkEventButton*)> sig = bind( &VRGuiContextMenu::on_widget_rightclick, this, placeholders::_1, menu );
    connect_signal(widget, sig, "button_release_event", true);
}
