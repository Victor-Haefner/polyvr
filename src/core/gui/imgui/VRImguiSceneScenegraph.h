#ifndef VRIMGUISCENESCENEGRAPH_H_INCLUDED
#define VRIMGUISCENESCENEGRAPH_H_INCLUDED

#include "VRImguiUtils.h"
#include "core/gui/VRGuiSignals.h"
#include "imWidgets/VRImguiTreeview.h"
#include "imWidgets/VRImguiVector.h"
#include "imWidgets/VRImguiColorPicker.h"
#include "imWidgets/VRImguiCombo.h"

using namespace std;

class ImScenegraph {
    private:
        ImTreeview tree;
        string title;
        string selected;

        string objType;

        string parent;
        string persistency;
        bool visible = true;
        bool pickable = false;
        bool castShadow = true;

        bool hasEntity = false;
        string entityName;
        string entityConcepts;
        vector<string> entityParams;
        vector<string> entityParamNames;

        Im_Vector position;
        Im_Vector atvector;
        Im_Vector direction;
        Im_Vector upvector;
        Im_Vector scale;
        Im_Vector constrTranslation;
        bool global = false;
        bool constrActive = false;
        bool constrLocal = false;
        Im_Vector constrDof1;
        Im_Vector constrDof2;
        Im_Vector constrDof3;
        Im_Vector constrDof4;
        Im_Vector constrDof5;
        Im_Vector constrDof6;
        bool doPhysicalize = false;
        bool physDynamic = false;

        bool doAcceptRoot = false;
        ImInput camAspect;
        ImInput camFov;
        ImInput camNear;
        ImInput camFar;
        int camProjection = 0;
        vector<const char*> camProjections;

        bool lightOn = true;
        int lightType = 0;
        vector<const char*> lightTypes;
        bool shadowsOn = true;
        int shadowResolution = true;
        vector<const char*> shadowResolutions;

        Im_Vector lodCenter;
        vector<double> lodDistances;

        string geoOrigin;
        vector<string> geoParams;
        vector<string> geoParamNames;
        vector<pair<string, int>> geoData;

        string matName;
        ImColorPicker matAmbient;
        ImColorPicker matDiffuse;
        ImColorPicker matSpecular;
        ImColorPicker matEmission;
        bool matLit;
        bool matMeshColors;
        ImCombo matPointsize;
        ImCombo matLinewidth;

        string texDims;
        string texSize;
        string texChannels;

    public:
        ImScenegraph();
        void render();

        void treeClear();
        void treeAppend(string ID, string label, string parent, string type, string cla, string mod, string col);

        void setupObject(OSG::VRGuiSignals::Options o);
        void setupEntity(OSG::VRGuiSignals::Options o);
        void setupTransform(OSG::VRGuiSignals::Options o);
        void setupCamera(OSG::VRGuiSignals::Options o);
        void setupLight(OSG::VRGuiSignals::Options o);
        void setupLod(OSG::VRGuiSignals::Options o);
        void setupGeometry(OSG::VRGuiSignals::Options o);
        void setupMaterial(OSG::VRGuiSignals::Options o);
};

#endif // VRIMGUISCENESCENEGRAPH_H_INCLUDED
