#ifndef VRGUISEMANTICS_H_INCLUDED
#define VRGUISEMANTICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/scene/VRSceneManager.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include "VRGuiSignals.h"

namespace Gtk { class Fixed; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiSemantics {
    private:
        Gtk::Fixed* canvas = 0;

        void on_new_clicked();
        void on_del_clicked();

        void on_treeview_select();

        void clearCanvas();
        void drawCanvas(string name);
        void drawConcept(VRConceptPtr concept, int x, int y);

    public:
        VRGuiSemantics();

        void update();
};

OSG_END_NAMESPACE

#endif // VRGUISEMANTICS_H_INCLUDED
