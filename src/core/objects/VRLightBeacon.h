#ifndef VRLIGHTBEACON_H_INCLUDED
#define VRLIGHTBEACON_H_INCLUDED

#include "VRTransform.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRLight;

class VRLightBeacon : public VRTransform {
    protected:
        VRLightPtr light;
        NodeRecPtr lightGeo;

        void saveContent(xmlpp::Element* e);
        void loadContent(xmlpp::Element* e);

    public:
        VRLightBeacon(string name);
        ~VRLightBeacon();

        static VRLightBeaconPtr create(string name);
        VRLightBeaconPtr ptr();

        VRLightPtr getLight();
        void setLight(VRLightPtr l);

        void showLightGeo(bool b);

        static vector<VRLightBeaconPtr>& getAll();
};

OSG_END_NAMESPACE;

#endif // VRLIGHTBEACON_H_INCLUDED
