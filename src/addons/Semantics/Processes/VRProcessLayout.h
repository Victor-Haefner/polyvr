#ifndef VRPROCESSLAYOUT_H_INCLUDED
#define VRPROCESSLAYOUT_H_INCLUDED

#include "../VRSemanticsFwd.h"
#include "core/objects/VRTransform.h"
#include "VRProcess.h"
#include "VRProcessEngine.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/objects/material/VRMaterial.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRProcessLayout : public VRTransform {
    private:
        VRProcessPtr process;
        VRProcessEnginePtr engine;
        VRPathtoolPtr toolSID;
        map<int, VRPathtoolPtr> toolSBDs;
        map<int, VRObjectWeakPtr> elements;
        map<VRObject*, int> elementIDs;
        float height = 2;
        float layoutScale = 1; // 0.005
        bool updatePaused = false;
        Color4f colorSubject = Color4f(0.8,0.9,1,1);
        Color4f colorMessage = Color4f(1,1,0,1);
        Color4f colorState = Color4f(1,0.9,0.8,1);
        Color4f colorActiveState = Color4f(1,0.51,0.22,1);
        Color4f colorSendState = Color4f(0.62,0.99,0.66,1);
        Color4f colorReceiveState = Color4f(0.96,0.47,0.5,1);

        VRUpdateCbPtr updateCb;

        VRGeometryPtr newWidget(VRProcessNodePtr n, float height);

        void init();
        void rebuild(); // TODO
        void build(VRProcessDiagramPtr diagram, VRPathtoolPtr pathtool, Vec3d position);
        void buildSID();
        void buildSBDs();
        void printHandlePositions();

        void appendToHandle(Vec3d pos, VRProcessNodePtr node, VRPathtoolPtr ptool);
        void setupLabel(VRProcessNodePtr message, VRPathtoolPtr ptool, vector<VRProcessNodePtr> nodes);

    public:
        VRProcessLayout(string name = "");
        ~VRProcessLayout();

        static VRProcessLayoutPtr create(string name = "");
        VRProcessLayoutPtr ptr();

        void setParameters(float height, float scale);
        void setProcess(VRProcessPtr p);
        VRProcessPtr getProcess();
        void setEngine(VRProcessEnginePtr e);
        VRObjectPtr getElement(int i);
        void remElement(VRObjectPtr o);
        int getElementID(VRObjectPtr o);
        VRProcessNodePtr getProcessNode(int i);
        VRObjectPtr addElement(VRProcessNodePtr n);
        void selectElement(VRGeometryPtr geo);
        void setElementName(int ID, string name);

        VRPathtoolPtr getSIDPathtool();
        VRPathtoolPtr getSBDPathtool(int sID);

        void storeLayout(string path);
        void loadLayout(string path);

        void update();
        void pauseUpdate(bool b);
        void updatePathtools();
};

OSG_END_NAMESPACE;

#endif // VRPROCESSLAYOUT_H_INCLUDED
