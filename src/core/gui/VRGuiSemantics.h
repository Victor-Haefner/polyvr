#ifndef VRGUISEMANTICS_H_INCLUDED
#define VRGUISEMANTICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/scene/VRSceneManager.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include "VRGuiSignals.h"

namespace Gtk {
    class Fixed;
    class Widget;
    class Label;
    class TreeView;
}

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiSemantics {
    private:
        struct ConceptWidget {
            Gtk::Widget* widget;
            Gtk::Label* label;
            Gtk::TreeView* treeview;
            VRConceptPtr concept;

            ConceptWidget(VRConceptPtr concept = 0);

            void on_select();
            void on_select_property();
        };

        Gtk::Fixed* canvas = 0;
        map<string, ConceptWidget> concepts;

        void on_new_clicked();
        void on_del_clicked();

        void on_treeview_select();
        void on_property_treeview_select();

        void clearCanvas();
        void drawCanvas(string name);

    public:
        VRGuiSemantics();

        void update();
};

OSG_END_NAMESPACE

#endif // VRGUISEMANTICS_H_INCLUDED
