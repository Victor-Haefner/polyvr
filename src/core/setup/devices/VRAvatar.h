#ifndef VRAVATAR_H_INCLUDED
#define VRAVATAR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>

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
        VRTransformPtr deviceRoot = 0;
        vector<Beacon> beacons;

        VRObjectPtr initRay();
        VRObjectPtr initCone();
        VRObjectPtr initBroadRay();

        void addAll();
        void hideAll();

    protected:
        VRAvatar(string name);
        ~VRAvatar();

        void addAvatar(VRObjectPtr geo);

    public:
        void enableAvatar(string avatar);
        void disableAvatar(string avatar);

        Beacon addBeacon();
        VRTransformPtr getBeacon(int i);
        VRTransformPtr editBeacon(int i);
        void setBeacon(VRTransformPtr b, int i);
        void updateBeacons();
};

OSG_END_NAMESPACE;

#endif // VRAVATAR_H_INCLUDED

// TODO: adjust class for vector<Beacon>, multiple beacons
