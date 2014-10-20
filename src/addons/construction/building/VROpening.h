#ifndef VRDOOR_H_INCLUDED
#define VRDOOR_H_INCLUDED

#include "core/objects/VRTransform.h"
#include "core/setup/devices/VRDevice.h"
#include "core/scene/VRScene.h"

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
        VRTransform *d1, *d2;
        VRFunction<Vec3f> *fkt1, *fkt2;
        VRDevCb* toggleCallback;
        VRSignal* sig;
        VRScene* scene;
        string sound;
        string param;

        void initAnimations(VRObject* _d1, VRObject* _d2);

    protected:
        VRObject* copy(vector<VRObject*> children);

    public:
        //VROpening(string name); // TODO -> deprecated??
        VROpening(string name, VRObject* obj, VRScene* _scene, VRSignal* _sig, string _param);

        void setSound(string s);

        void open();

        void close();

        void toggle(VRDevice* dev);
};

OSG_END_NAMESPACE;

#endif // VRDOOR_H_INCLUDED
