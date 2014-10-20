#ifndef VRGUICONTEXTMENU_H_INCLUDED
#define VRGUICONTEXTMENU_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/scene/VRSceneManager.h"
#include "VRGuiSignals.h"
#include <gtkmm/treestore.h>
#include <gtkmm/menu.h>

using namespace std;

class VRGuiContextMenu {
    private:
        map< string, Gtk::Menu* > menus;

    public:
        VRGuiContextMenu(string name);

        void appendItem( string menu, string item, const sigc::slot<void>& fkt );
        void appendMenu( string menu, string item, string new_menu );
        void popup(string menu, GdkEventButton* event);

        bool on_widget_rightclick(GdkEventButton * event, string menu);
        void connectWidget(string menu, Glib::RefPtr<Gtk::Widget> widget);
        void connectWidget(string menu, Gtk::Widget* widget);
};

#endif // VRGUICONTEXTMENU_H_INCLUDED
