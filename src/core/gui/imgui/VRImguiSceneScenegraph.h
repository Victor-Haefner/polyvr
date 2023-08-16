#ifndef VRIMGUISCENESCENEGRAPH_H_INCLUDED
#define VRIMGUISCENESCENEGRAPH_H_INCLUDED

#include "VRImguiUtils.h"
#include "core/gui/VRGuiSignals.h"
#include "imWidgets/VRImguiTreeview.h"
#include "imWidgets/VRImguiVector.h"

using namespace std;

class ImScenegraph {
    private:
        ImTreeview tree;

        Im_Vector position;
        Im_Vector atvector;
        Im_Vector direction;
        Im_Vector upvector;
        Im_Vector scale;


        string title;
        string selected;

        string parent;
        string persistency;
        bool visible;
        bool pickable;
        bool castShadow;

    public:
        ImScenegraph();
        void render();

        void treeClear();
        void treeAppend(string ID, string label, string parent, string type, string cla, string mod, string col);

        void setupObject(OSG::VRGuiSignals::Options o);
        void setupTransform(OSG::VRGuiSignals::Options o);
};

#endif // VRIMGUISCENESCENEGRAPH_H_INCLUDED
