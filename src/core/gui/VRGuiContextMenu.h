#ifndef VRGUICONTEXTMENU_H_INCLUDED
#define VRGUICONTEXTMENU_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <functional>
#include "VRGuiSignals.h"

using namespace std;

struct _GtkMenu;
struct _GdkEventButton;
struct _GtkWidget;

class VRGuiContextMenu {
    private:
        map< string, _GtkMenu* > menus;

    public:
        VRGuiContextMenu(string name);

        void appendItem( string menu, string item, function<void()> fkt );
        void appendMenu( string menu, string item, string new_menu );
        void popup(string menu, _GdkEventButton* event);

        bool on_widget_rightclick(_GdkEventButton * event, string menu);
        void connectWidget(string menu, _GtkWidget* widget);
};

#endif // VRGUICONTEXTMENU_H_INCLUDED
