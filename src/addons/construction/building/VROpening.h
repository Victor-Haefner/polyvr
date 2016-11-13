#ifndef VRDOOR_H_INCLUDED
#define VRDOOR_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/objects/VRTransform.h"
#include "core/setup/devices/VRDevice.h"
#include "core/scene/VRScene.h"
#include <memory>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VROCtoggle {
    public:
        enum OCState { OPEN, CLOSE };

    protected:
        OCState state;

    public:
        VROCtoggle();

        bool isClose();
        bool isOpen();
};

class VROpening: public VRTransform, public VROCtoggle {
    private:
        VRTransformPtr d1, d2;
        shared_ptr< VRFunction<Vec3f> > fkt1;
        shared_ptr< VRFunction<Vec3f> > fkt2;
        VRDeviceCbPtr toggleCallback;
        VRSignalPtr sig;
        VRScene* scene;
        string sound;
        string param;

        void initAnimations(VRObjectPtr _d1, VRObjectPtr _d2);

    protected:
        VRObjectPtr copy(vector<VRObjectPtr> children);

    public:
        //VROpening(string name); // TODO -> deprecated??
        VROpening(string name, VRObjectPtr obj, VRScene* _scene, VRSignalPtr _sig, string _param);
        static VROpeningPtr create(string name, VRObjectPtr obj, VRScene* _scene, VRSignalPtr _sig, string _param);
        VROpeningPtr ptr();

        void setSound(string s);

        void open();

        void close();

        void toggle(VRDeviceWeakPtr dev);
};

OSG_END_NAMESPACE;

#endif // VRDOOR_H_INCLUDED
