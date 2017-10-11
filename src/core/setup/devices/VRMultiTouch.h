#ifndef VRMULTITOUCH_H_INCLUDED
#define VRMULTITOUCH_H_INCLUDED

#include "core/setup/windows/VRView.h"
#include "VRDevice.h"
#include <OpenSG/OSGLine.h>

/*
not compiling? then you are missing a library:
 sudo apt-get install libmtdev-dev
*/

#include <mtdev.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRMultiTouch : public VRDevice {
    public:
        struct Touch {
            int key;
            Vec3i pos;
            Touch(int k = -1);
        };

    private:
        int fd;
        mtdev dev;
        VRUpdateCbPtr updatePtr;
        VRCameraWeakPtr cam;
        VRViewWeakPtr view;
        Line ray;

        void multFull(Matrix _matrix, const Pnt3f &pntIn, Pnt3f  &pntOut);
        bool calcViewRay(VRCameraPtr pcam, Line &line, float x, float y, int W, int H);

        string device = "/dev/input/event5"; // TODO: make it configurable!
        map<int, Touch> fingers;
        int currentTouchID = -1;

    public:
        VRMultiTouch();
        ~VRMultiTouch();

        static VRMultiTouchPtr create();
        VRMultiTouchPtr ptr();

        void updateDevice();
        void connectDevice();

        void clearSignals();

        //3d object to emulate a hand in VRSpace
        void updatePosition(int x, int y);

        void mouse(int button, int state, int x, int y);
        void motion(int x, int y);

        Line getRay();
        void setCamera(VRCameraPtr cam);
        void setViewport(VRViewPtr view);

        void save(xmlpp::Element* e);
        void load(xmlpp::Element* e);
};

OSG_END_NAMESPACE;

#endif // VRMULTITOUCH_H_INCLUDED
