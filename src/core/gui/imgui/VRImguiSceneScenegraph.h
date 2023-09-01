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
        string title;
        string selected;

        string parent;
        string persistency;
        bool visible = true;
        bool pickable = false;
        bool castShadow = true;

        Im_Vector position;
        Im_Vector atvector;
        Im_Vector direction;
        Im_Vector upvector;
        Im_Vector scale;
        Im_Vector constrTranslation;
        bool global = false;
        bool doConstrTranslation = false;
        bool doConstrRotation = false;
        int cTransMode = 0;
        bool cRotX = false;
        bool cRotY = false;
        bool cRotZ = false;
        bool doPhysicalize = false;
        bool doAcceptRoot = false;
        bool physDynamic = false;
        ImInput camAspect;
        ImInput camFov;
        ImInput camNear;
        ImInput camFar;

        Im_Vector lodCenter;
        vector<double> lodDistances;

    public:
        ImScenegraph();
        void render();

        void treeClear();
        void treeAppend(string ID, string label, string parent, string type, string cla, string mod, string col);

        void setupObject(OSG::VRGuiSignals::Options o);
        void setupTransform(OSG::VRGuiSignals::Options o);
        void setupCamera(OSG::VRGuiSignals::Options o);
        void setupLight(OSG::VRGuiSignals::Options o);
        void setupLod(OSG::VRGuiSignals::Options o);
        void setupGeometry(OSG::VRGuiSignals::Options o);
};

#endif // VRIMGUISCENESCENEGRAPH_H_INCLUDED
