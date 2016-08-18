#ifndef VRGUISEMANTICS_H_INCLUDED
#define VRGUISEMANTICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include "core/scene/VRSceneManager.h"
#include "core/utils/VRFunctionFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include "VRGuiSignals.h"

#include "gtkmm/treemodel.h"

namespace Gtk {
    class Fixed;
    class Frame;
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
            Vec2f pos;

            VRGuiSemantics* manager = 0;
            Gtk::Fixed* canvas = 0;
            Gtk::Frame* widget = 0;
            Gtk::Label* label = 0;
            Gtk::TreeView* treeview = 0;
            VRConceptPtr concept;
            VRPropertyPtr selected_property;

            ConceptWidget(VRGuiSemantics* m, Gtk::Fixed* canvas = 0, VRConceptPtr concept = 0);
            ~ConceptWidget();
            void update();

            void on_select();
            void on_select_property();
            void on_rem_clicked();
            void on_rem_prop_clicked();
            void on_new_clicked();
            void on_edit_clicked();
            void on_newp_clicked();
            void on_edit_prop_clicked();
            bool on_expander_clicked(GdkEventButton* e);

            void move(Vec2f p);
            Vec2f getAnchorPoint(Vec2f p);
            void setPropRow(Gtk::TreeModel::iterator iter, string name, string type, string color, int flag);
        };

        typedef shared_ptr<ConceptWidget> ConceptWidgetPtr;
        typedef weak_ptr<ConceptWidget> ConceptWidgetWeakPtr;

        struct ConnectorWidget {
            Gtk::Separator* sh1 = 0;
            Gtk::Separator* sh2 = 0;
            Gtk::Separator* sv1 = 0;
            Gtk::Separator* sv2 = 0;
            Gtk::Fixed* canvas = 0;
            ConceptWidgetWeakPtr w1;
            ConceptWidgetWeakPtr w2;

            ConnectorWidget(Gtk::Fixed* canvas = 0);

            void set(ConceptWidgetPtr w1, ConceptWidgetPtr w2);

            void update();
        };

        typedef shared_ptr<ConnectorWidget> ConnectorWidgetPtr;
        typedef weak_ptr<ConnectorWidget> ConnectorWidgetWeakPtr;

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
        void remConcept(ConceptWidget* w);

        VRSemanticManagerPtr getManager();

    public:
        VRGuiSemantics();

        void updateOntoList();
        void updateCanvas();
};

OSG_END_NAMESPACE

#endif // VRGUISEMANTICS_H_INCLUDED
