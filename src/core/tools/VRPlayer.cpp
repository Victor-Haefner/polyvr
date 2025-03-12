#include "VRPlayer.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"

double clamp(double f, double a = -1, double b = 1) { return f<a ? a : f>b ? b : f; }

using namespace OSG;

VRPlayer::VRPlayer() {
    updateCb = VRUpdateCb::create( "player", bind(&VRPlayer::update, this));
    VRScene::getCurrent()->addUpdateFkt(updateCb);
}

VRPlayer::~VRPlayer() {}

VRPlayerPtr VRPlayer::create() { return VRPlayerPtr( new VRPlayer() ); }
VRPlayerPtr VRPlayer::ptr() { return static_pointer_cast<VRPlayer>(shared_from_this()); }

void VRPlayer::setCallback(VRAnimCbPtr cb) { callback = cb; }
void VRPlayer::setLoop(bool b) { loop = b; }

void VRPlayer::reset() { speed = 0; moveTo(0); }
void VRPlayer::pause() { speed = 0; }
void VRPlayer::play(double s) { speed = s; }
void VRPlayer::moveTo(double p) { progress = clamp(p, 0.0, 1.0); (*callback)(p); }

void VRPlayer::update() {
    if (abs(speed) < 1e-6) return;

    double t = getTime()*1e-6; // seconds
    auto dt = t-lastUpdateTime;
    lastUpdateTime = t;

    double p = progress + speed*dt;
    if (p > 1.0 && loop) p -= 1.0;
    p = clamp(p, 0.0, 1.0);
    if (abs(p-0.0) < 1e-6) p = 0.0;
    if (abs(p-1.0) < 1e-6) p = 1.0;

    if (abs(progress-p) < 1e-9) return;
    progress = p;
    (*callback)(p);
}
