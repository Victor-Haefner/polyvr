#ifndef VRGUISEMANTICS_H_INCLUDED
#define VRGUISEMANTICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include "core/scene/VRSceneManager.h"
#include "core/utils/VRFunctionFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include "VRGuiSignals.h"

#include "widgets/VRSemanticWidget.h"
#include "widgets/VRConceptWidget.h"

namespace Gtk {
    class Separator;
}

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiSemantics {
    public:
        struct EntityWidget : public VRSemanticWidget {
            VREntityPtr entity;

            EntityWidget(VRGuiSemantics* m, Gtk::Fixed* canvas = 0, VREntityPtr concept = 0);
            void update();
        };

        struct RuleWidget : public VRSemanticWidget{
            VROntologyRulePtr rule;

            RuleWidget(VRGuiSemantics* m, Gtk::Fixed* canvas = 0, VROntologyRulePtr concept = 0);
            void update();
        };

        typedef shared_ptr<VRSemanticWidget> VRSemanticWidgetPtr;
        typedef weak_ptr<VRSemanticWidget> VRSemanticWidgetWeakPtr;
        typedef shared_ptr<VRConceptWidget> VRConceptWidgetPtr;
        typedef weak_ptr<VRConceptWidget> VRConceptWidgetWeakPtr;
        typedef shared_ptr<EntityWidget> EntityWidgetPtr;
        typedef weak_ptr<EntityWidget> EntityWidgetWeakPtr;
        typedef shared_ptr<RuleWidget> RuleWidgetPtr;
        typedef weak_ptr<RuleWidget> RuleWidgetWeakPtr;

        struct ConnectorWidget {
            Gtk::Separator* sh1 = 0;
            Gtk::Separator* sh2 = 0;
            Gtk::Separator* sv1 = 0;
            Gtk::Separator* sv2 = 0;
            Gtk::Fixed* canvas = 0;
            VRConceptWidgetWeakPtr w1;
            VRConceptWidgetWeakPtr w2;

            ConnectorWidget(Gtk::Fixed* canvas = 0);

            void set(VRConceptWidgetPtr w1, VRConceptWidgetPtr w2);

            void update();
        };

        typedef shared_ptr<ConnectorWidget> ConnectorWidgetPtr;
        typedef weak_ptr<ConnectorWidget> ConnectorWidgetWeakPtr;

    private:
        Gtk::Fixed* canvas = 0;
        map<string, VRSemanticWidgetPtr> widgets;
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

        VRSemanticManagerPtr getManager();

    public:
        VRGuiSemantics();

        VROntologyPtr getSelectedOntology();

        void updateOntoList();
        void updateCanvas();

        void copyConcept(VRConceptWidget* w);
        void remConcept(VRConceptWidget* w);
};

OSG_END_NAMESPACE

#endif // VRGUISEMANTICS_H_INCLUDED
