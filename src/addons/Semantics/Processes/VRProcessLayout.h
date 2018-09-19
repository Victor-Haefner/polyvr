#ifndef VRPROCESSLAYOUT_H_INCLUDED
#define VRPROCESSLAYOUT_H_INCLUDED

#include "../VRSemanticsFwd.h"
#include "core/objects/VRTransform.h"
#include "VRProcess.h"

#include "core/utils/VRFunctionFwd.h"

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

        VRUpdateCbPtr updateCb;

        VRGeometryPtr newWidget(VRProcessNodePtr n, float height);

        void init();
        void rebuild(); // TODO
        void buildSID();
        void buildSBDs();

    public:
        VRProcessLayout(string name = "");
        ~VRProcessLayout();

        static VRProcessLayoutPtr create(string name = "");
        VRProcessLayoutPtr ptr();

        void setProcess(VRProcessPtr p);
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

        void update();
};

OSG_END_NAMESPACE;

#endif // VRPROCESSLAYOUT_H_INCLUDED
