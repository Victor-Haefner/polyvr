#ifndef VRAVATAR_H_INCLUDED
#define VRAVATAR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include <vector>

#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRAvatar {
    public:
        struct Beacon {
            VRTransformPtr tmpContainer;
            VRTransformPtr beacon;
            map<string, VRObjectPtr> avatars;
        };

    private:
        string deviceName;
        VRTransformPtr deviceRoot = 0;
        vector<Beacon> beacons;

        VRObjectPtr initRay(int beaconID);
        VRObjectPtr initCone();
        VRObjectPtr initBroadRay();

        void addAll(int i);
        void hideAll(int i);

    protected:
        VRAvatar(string name);
        ~VRAvatar();

        void addAvatar(VRObjectPtr geo, int i = 0);

    public:
        void enableAvatar(string avatar, int i = 0);
        void disableAvatar(string avatar, int i = 0);

        int addBeacon();
        VRTransformPtr getBeaconRoot();
        VRTransformPtr getBeacon(int i = 0);
        VRTransformPtr editBeacon(int i = 0);
        void setBeacon(VRTransformPtr b, int i = 0);
        void updateBeacons();
};

OSG_END_NAMESPACE;

#endif // VRAVATAR_H_INCLUDED
