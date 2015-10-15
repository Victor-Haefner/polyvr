#ifndef VRAVATAR_H_INCLUDED
#define VRAVATAR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>

#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRAvatar {
    private:
        VRTransformPtr deviceRoot = 0;
        VRTransformPtr tmpContainer = 0;
        map<string, VRObjectPtr> avatars;

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

        VRTransformPtr getBeacon();
        VRTransformPtr editBeacon();
        void setBeacon(VRTransformPtr b);
        void updateBeacon();
};

OSG_END_NAMESPACE;

#endif // VRAVATAR_H_INCLUDED
