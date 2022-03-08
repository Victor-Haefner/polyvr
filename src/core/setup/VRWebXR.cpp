#include "VRWebXR.h"
#include "core/math/pose.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetup.h"
#include "core/objects/VRCamera.h"

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGQuaternion.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
typedef const char* CSTR;
#endif


/**
- intersection auf reale welt, "hit test"
- marker detection (python feeds image, cpp sets javascript texture to webxr api, cpp setpose for tracking)
- depth map
- fix camera headtracking, maybe use HeadTrackedStereoCameraDecorator in VRView?
    - work around: in VRGlutWindow, multiply head transform on active cam just before rendering, then after rendering set the cam transform back
*/

using namespace OSG;

VRWebXR::VRWebXR() : VRDevice("webxr") {}
VRWebXR::~VRWebXR() {}

VRWebXRPtr VRWebXR::create() {
    auto xr = VRWebXRPtr( new VRWebXR() );
    xr->init();
    return xr;
}

VRWebXRPtr VRWebXR::ptr() { return static_pointer_cast<VRWebXR>(shared_from_this()); }

void VRWebXR::init() {
        //trackers[ID] = VR.find(ID);
}

PosePtr VRWebXR::toPose(float x, float y, float z, float qx, float qy, float qz, float qw) {
    Quaterniond q(Vec3d(qx, qy, qz), qw);
    Matrix4d m;
    m.setRotate(q);
    m.setTranslate(x,y,z);
    return Pose::create(m);
}

void VRWebXR::setPose(string ID, float x, float y, float z, float qx, float qy, float qz, float qw) {
    PosePtr tPose = toPose(x,y,z,qx,qy,qz,qw);
    if (ID == "room") {
        room = tPose;
        room->invert();
        return;
    }

    if (room) tPose = room->multRight(tPose);

    if (ID == "head") {
        //VRScene::getCurrent()->getActiveCamera()->setPose( tPose );
        head = tPose;
        return;
    }

    if (trackers.count(ID)) {
        trackers[ID]->setPose( tPose );
        return;
    }
}

void VRWebXR::preRender() {
    if (head) {
        auto cam = VRScene::getCurrent()->getActiveCamera();
        tempCam = cam->getPose();
        auto pCam = tempCam->multRight(head);
        cam->setPose(pCam);
    }
}

void VRWebXR::postRender() {
    if (tempCam) {
        auto cam = VRScene::getCurrent()->getActiveCamera();
        cam->setPose(tempCam);
    }
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE void WebXR_setPose(CSTR ID, float x, float y, float z, float qx, float qy, float qz, float qw) {
    auto webxr = dynamic_pointer_cast<VRWebXR>( VRSetup::getCurrent()->getDevice("webxr") );
    webxr->setPose(ID,x,y,z,qx,qy,qz,qw);
}
#endif
