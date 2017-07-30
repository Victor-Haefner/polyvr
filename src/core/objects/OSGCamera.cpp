#include "OSGCamera.h"

using namespace OSG;

OSGCamera::OSGCamera(CameraMTRecPtr cam) {
    this->cam = cam;
}

OSGCameraPtr OSGCamera::create(CameraMTRecPtr cam) { return OSGCameraPtr( new OSGCamera(cam) ); }
