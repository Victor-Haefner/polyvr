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

        VRObjectPtr initRay();
        VRObjectPtr initCone();
        VRObjectPtr initBroadRay();

        void addAll(int i);
        void hideAll(int i);

    protected:
        VRAvatar(string name);
        ~VRAvatar();

        void addAvatar(VRObjectPtr geo, int i);

    public:
        void enableAvatar(string avatar, int i);
        void enableAvatar(string avatar);
        void disableAvatar(string avatar, int i);
        void disableAvatar(string avatar);

        void addBeacon();
        VRTransformPtr getBeaconRoot();
        VRTransformPtr getBeacon(int i);
        VRTransformPtr getBeacon();
        VRTransformPtr editBeacon(int i);
        VRTransformPtr editBeacon();
        void setBeacon(VRTransformPtr b, int i);
        void setBeacon(VRTransformPtr b);
        void updateBeacons();
};

OSG_END_NAMESPACE;

#endif // VRAVATAR_H_INCLUDED
