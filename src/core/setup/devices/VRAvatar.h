#ifndef VRAVATAR_H_INCLUDED
#define VRAVATAR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObject;
class VRTransform;

class VRAvatar {
    private:
        VRTransform* deviceRoot;
        map<string, VRObject*> avatars;

        VRObject* initRay();

        VRObject* initCone();

        VRObject* initBroadRay();

        void addAll();
        void hideAll();

    protected:
        VRAvatar(string name);
        ~VRAvatar();

        void addAvatar(VRObject* geo);

    public:
        void enableAvatar(string avatar);
        void disableAvatar(string avatar);

        VRTransform* getBeacon();
        void setBeacon(VRTransform* b);
};

OSG_END_NAMESPACE;

#endif // VRAVATAR_H_INCLUDED
