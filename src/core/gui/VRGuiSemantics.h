#ifndef VRGUISEMANTICS_H_INCLUDED
#define VRGUISEMANTICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/scene/VRSceneManager.h"
#include "core/utils/VRFunctionFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include "VRGuiSignals.h"

#include "gtkmm/treemodel.h"

namespace Gtk {
    class Fixed;
    class Widget;
    class Label;
    class TreeView;
    class Separator;
}

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiSemantics {
    public:
        struct ConceptWidget {
            float x, y;

            VRGuiSemantics* manager = 0;
            Gtk::Fixed* canvas = 0;
            Gtk::Widget* widget = 0;
            Gtk::Label* label = 0;
            Gtk::TreeView* treeview = 0;
            VRConceptPtr concept;
            VRPropertyPtr selected_property;

            ConceptWidget(VRGuiSemantics* m, Gtk::Fixed* canvas = 0, VRConceptPtr concept = 0);
            void update();

            void on_select();
            void on_select_property();
            void on_rem_clicked();
            void on_rem_prop_clicked();
            void on_new_clicked();
            void on_edit_clicked();
            void on_newp_clicked();
            bool on_expander_clicked(GdkEventButton* e);

            void move(float x, float y);
            void setPropRow(Gtk::TreeModel::iterator iter, string name, string type, string color, int flag);
        };

        struct ConnectorWidget {
            Gtk::Separator* s1 = 0;
            Gtk::Separator* s2 = 0;
            Gtk::Separator* s3 = 0;
            Gtk::Fixed* canvas = 0;

            ConnectorWidget(Gtk::Fixed* canvas = 0);

            void set(float x1, float y1, float x2, float y2);
        };

        typedef shared_ptr<ConceptWidget> ConceptWidgetPtr;
        typedef shared_ptr<ConnectorWidget> ConnectorWidgetPtr;

    private:
        Gtk::Fixed* canvas = 0;
        map<string, ConceptWidgetPtr> concepts;
        map<string, ConnectorWidgetPtr> connectors;

        VROntologyPtr current;
        VRUpdatePtr updateLayoutCb;

        void on_new_clicked();
        void on_del_clicked();

        void on_treeview_select();
        void on_property_treeview_select();

        void clearCanvas();
        void setOntology(string name);
        void updateLayout();

        void copyConcept(ConceptWidget* w);

    public:
        VRGuiSemantics();

        void updateOntoList();
        void updateCanvas();
};

OSG_END_NAMESPACE

#endif // VRGUISEMANTICS_H_INCLUDED
