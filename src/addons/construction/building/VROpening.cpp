#include "VROpening.h"
#include "core/scene/VRSoundManager.h"
#include "core/scene/VRAnimationManagerT.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


VROCtoggle::VROCtoggle() : state(CLOSE) {;}

bool VROCtoggle::isClose() { return (state == CLOSE); }
bool VROCtoggle::isOpen() { return (state == OPEN); }


void VROpening::initAnimations(VRObject* _d1, VRObject* _d2) {
    // Door Animation
    fkt1 = 0;
    fkt2 = 0;
    d1 = (VRTransform*)_d1;
    d2 = (VRTransform*)_d2;

    if (d2 == 0 and param == "CW") {
        d2 = d1;
        d1 = 0;
    }

    if (d1) fkt1 = new VRFunction<Vec3f>("3DEntSetUp", boost::bind(&VRTransform::setUp, d1, _1));
    if (d2) fkt2 = new VRFunction<Vec3f>("3DEntSetUp", boost::bind(&VRTransform::setUp, d2, _1));
}

VRObject* VROpening::copy(vector<VRObject*> children) {
    VROpening* d = new VROpening(getBaseName(), 0, scene, sig, param);

    VRObject *d1 = 0;
    VRObject *d2 = 0;
    for (uint i=0;i<children.size();i++) {
        if (d1 == 0) d1 = children[i]->find("opening");
        if (d2 == 0) d2 = children[i]->find("opening2");
    }

    d->initAnimations(d1, d2);

    d->state = state;
    d->sound = sound;

    return d;
}

VROpening::VROpening(string name, VRObject* obj, VRScene* _scene, VRSignal* _sig, string _param): VRTransform(name) {
    scene = _scene;
    sig = _sig;
    param = _param;

    // toggle callback
    toggleCallback = new VRDevCb("OpeningToggle", boost::bind(&VROpening::toggle, this, _1));

    if (obj) {
        addChild(obj);
        initAnimations(find("opening"), find("opening2"));
    }

    if (sig) sig->add(toggleCallback);
}

void VROpening::setSound(string s) { sound = s; }

void VROpening::open() {
    if (state == OPEN) return;
    state = OPEN;

    if (fkt1) scene->addAnimation(1, 0, fkt1, Vec3f(0,1,0), Vec3f(-1,0,0), false);
    if (fkt2) scene->addAnimation(1, 0, fkt2, Vec3f(0,1,0), Vec3f(1,0,0), false);
    VRSoundManager::get().playSound(sound);
}

void VROpening::close() {
    if (state == CLOSE) return;
    state = CLOSE;

    if (fkt1) scene->addAnimation(1, 0, fkt1, Vec3f(-1,0,0), Vec3f(0,1,0), false);
    if (fkt2) scene->addAnimation(1, 0, fkt2, Vec3f(1,0,0), Vec3f(0,1,0), false);
    VRSoundManager::get().playSound(sound);
}

void VROpening::toggle(VRDevice* dev) {
    if (d1 == 0 and d2 == 0) return;

    if (dev != 0) { //if triggered by a device, check if this is hit
        VRIntersection ins = dev->intersect(this);
        if (!ins.hit) return;
        if ( ins.object == 0 ) return;

        bool b = true;
        if (d1) if (d1->find(ins.object)) b=false;
        if (d2) if (d2->find(ins.object)) b=false;
        if (b) return;
    }

    if (state == CLOSE) open();
    else close();
}

OSG_END_NAMESPACE;
