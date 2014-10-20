#ifndef VRLIGHTBEACON_H_INCLUDED
#define VRLIGHTBEACON_H_INCLUDED

#include "VRTransform.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRLight;

class VRLightBeacon : public VRTransform {
    protected:
        VRLight* light;
        NodeRecPtr lightGeo;

        void saveContent(xmlpp::Element* e);
        void loadContent(xmlpp::Element* e);

    public:
        VRLightBeacon(string name);
        ~VRLightBeacon();

        VRLight* getLight();
        void setLight(VRLight* l);

        void showLightGeo(bool b);

        static vector<VRLightBeacon*>& getAll();
};

OSG_END_NAMESPACE;

#endif // VRLIGHTBEACON_H_INCLUDED
