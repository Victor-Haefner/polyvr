#ifndef OSGCAMERA_H_INCLUDED
#define OSGCAMERA_H_INCLUDED

#include <OpenSG/OSGCamera.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class OSGCamera {
    public:
        CameraMTRecPtr cam;
        OSGCamera(CameraMTRecPtr cam = 0);
        static OSGCameraPtr create(CameraMTRecPtr cam = 0);
};

OSG_END_NAMESPACE;

#endif // OSGCAMERA_H_INCLUDED
