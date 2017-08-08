#include "VRSpaceWarper.h"
#include "../objects/VRCamera.h"

using namespace OSG;

VRSpaceWarper::VRSpaceWarper() {}
VRSpaceWarper::~VRSpaceWarper() {}

VRSpaceWarperPtr VRSpaceWarper::create() { return VRSpaceWarperPtr(new VRSpaceWarper()); }
VRSpaceWarperPtr VRSpaceWarper::ptr() { return shared_from_this(); }

void VRSpaceWarper::warp(Matrix4d& m) { // enable in VRCameraManager.cpp
    auto p = Vec3d(m[3]);
    double K = 1e-6;
    m.setTranslate(p*K); // warp position
    Matrix4d ms; // warp scale
    ms.setScale(K);
    m.mult(ms);
}
