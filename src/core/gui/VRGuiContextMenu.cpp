#include "VRGuiContextMenu.h"
#include "VRGuiUtils.h"

#include <gtkmm/treestore.h>
#include <gtkmm/menu.h>

VRGuiContextMenu::VRGuiContextMenu(string name) {
    ;
}

bool signal_activated_(GdkEventButton* event, const sigc::slot<void>& fkt) {
    fkt();
    return true;
}

void on_item_select(Gtk::Menu* m) { cout << "\nMenu selected: " << m << endl; }

void VRGuiContextMenu::appendItem( string menu, string item, const sigc::slot<void>& fkt ) {
    if (menus.count(menu) == 0) menus[menu] = Gtk::manage(new Gtk::Menu());
    Gtk::Menu* m = menus[menu];
    Gtk::MenuItem* mi = Gtk::manage(new Gtk::MenuItem(item));

    //mi->signal_activate().connect(fkt, true); // does not work for submenus
    //mi->signal_select().connect( sigc::bind(&on_item_select, m) );

    mi->signal_button_press_event().connect( sigc::bind(&signal_activated_, fkt) );

    m->add( *mi );
    m->show_all();
}

void VRGuiContextMenu::appendMenu( string menu, string item, string new_menu ) {
    if (menus.count(menu) == 0) menus[menu] = Gtk::manage(new Gtk::Menu());

    Gtk::MenuItem* mi = Gtk::manage(new Gtk::MenuItem(item, true));
    if (menus.count(new_menu) == 0) menus[new_menu] = Gtk::manage(new Gtk::Menu());
    mi->set_submenu( *menus[new_menu] );

    menus[menu]->add( *mi );
    menus[menu]->show_all();
}

void VRGuiContextMenu::popup(string menu, GdkEventButton* event) {
    if (menus.count(menu) == 0) { cout << "\nVRGuiContextMenu::popup, no such menu: " << menu << endl; return; }
    menus[menu]->popup(event->button, event->time);
}

bool VRGuiContextMenu::on_widget_rightclick(GdkEventButton * event, string menu) {
    if (event->type != GDK_BUTTON_RELEASE) return false;
    if (event->button != 3) return false;
    popup(menu, event); return true;
}

void VRGuiContextMenu::connectWidget(string menu, Glib::RefPtr<Gtk::Widget> widget) {
    widget->add_events((Gdk::EventMask)GDK_BUTTON_PRESS_MASK);
    widget->add_events((Gdk::EventMask)GDK_BUTTON_RELEASE_MASK);
    widget->signal_button_release_event().connect( sigc::bind( sigc::mem_fun(*this, &VRGuiContextMenu::on_widget_rightclick), menu) );
}

void VRGuiContextMenu::connectWidget(string menu, Gtk::Widget* widget) {
    widget->add_events((Gdk::EventMask)GDK_BUTTON_PRESS_MASK);
    widget->add_events((Gdk::EventMask)GDK_BUTTON_RELEASE_MASK);
    widget->signal_button_release_event().connect( sigc::bind( sigc::mem_fun(*this, &VRGuiContextMenu::on_widget_rightclick), menu) );
}
