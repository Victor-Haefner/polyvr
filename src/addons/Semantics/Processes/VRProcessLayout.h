#ifndef VRPROCESSLAYOUT_H_INCLUDED
#define VRPROCESSLAYOUT_H_INCLUDED

#include "../VRSemanticsFwd.h"
#include "core/objects/VRTransform.h"
#include "VRProcess.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRProcessLayout : public VRTransform {
    private:
        VRProcessPtr process;
        map<int, VRObjectWeakPtr> elements;
        map<VRObject*, int> elementIDs;
        float height = 2;

        VRGeometryPtr newWidget(VRProcessNodePtr n, float height);

        void rebuild(); // TODO

    public:
        VRProcessLayout(string name = "");
        ~VRProcessLayout();

        static VRProcessLayoutPtr create(string name = "");
        VRProcessLayoutPtr ptr();

        //void setProcess(VRProcessPtr p);
        void setProcess(VRProcessPtr p);
        VRObjectPtr getElement(int i);
        int getElementID(VRObjectPtr o);
        VRProcessNodePtr getProcessNode(int i);
        VRObjectPtr addElement(VRProcessNodePtr n);
        void selectElement(VRGeometryPtr geo);
        void setElementName(int ID, string name);
};

OSG_END_NAMESPACE;

#endif // VRPROCESSLAYOUT_H_INCLUDED
