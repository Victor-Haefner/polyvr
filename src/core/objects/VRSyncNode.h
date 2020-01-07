#ifndef VRSyncNode_H_INCLUDED
#define VRSyncNode_H_INCLUDED

#include "VRTransform.h"
#include <OpenSG/OSGChangeList.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRLight;

class VRSyncNode : public VRTransform {
    protected:
        VRLightWeakPtr light;
        string light_name;
        OSGObjectPtr lightGeo;

        VRObjectPtr copy(vector<VRObjectPtr> children);

    public:
        VRSyncNode(string name);
        ~VRSyncNode();

        static VRSyncNodePtr create(string name = "None");
        VRSyncNodePtr ptr();

        VRLightWeakPtr getLight();
        void setLight(VRLightPtr l);

        void showLightGeo(bool b);

        static vector<VRSyncNodeWeakPtr>& getAll();

        void printChangeList();
};

OSG_END_NAMESPACE;

#endif // VRSyncNode_H_INCLUDED
