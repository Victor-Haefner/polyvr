#ifndef VRGUISEMANTICS_H_INCLUDED
#define VRGUISEMANTICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include "core/scene/VRSceneManager.h"
#include "core/utils/VRFunctionFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include "VRGuiSignals.h"
#include "VRGuiFwd.h"

namespace Gtk {
    class Fixed;
}

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiSemantics {
    private:
        Gtk::Fixed* canvas = 0;
        map<int, VRSemanticWidgetPtr> widgets;
        map<int, VRConnectorWidgetPtr> connectors;

        VROntologyPtr current;
        VRUpdatePtr updateLayoutCb;

        void on_new_clicked();
        void on_del_clicked();

        void on_treeview_select();
        void on_property_treeview_select();
        void on_query_clicked();

        void clearCanvas();
        void setOntology(string name);
        void updateLayout();

        void connect(VRSemanticWidgetPtr p1, VRSemanticWidgetPtr p2, string color);
        void disconnect(VRSemanticWidgetPtr p1, VRSemanticWidgetPtr p2);
        void disconnectAny(VRSemanticWidgetPtr p1);

        VRSemanticManagerPtr getManager();

    public:
        VRGuiSemantics();

        VROntologyPtr getSelectedOntology();

        void updateOntoList();
        void updateCanvas();

        void copyConcept(VRConceptWidget* w);
        void addEntity(VRConceptWidget* w);
        void addRule(VRConceptWidget* w);
        void remConcept(VRConceptWidget* w);
        void remEntity(VREntityWidget* w);
        void remRule(VRRuleWidget* w);
};

OSG_END_NAMESPACE

#endif // VRGUISEMANTICS_H_INCLUDED
