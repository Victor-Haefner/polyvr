#ifndef VRMULTITOUCH_H_INCLUDED
#define VRMULTITOUCH_H_INCLUDED

#include "core/setup/windows/VRView.h"
#include "VRDevice.h"
#include <OpenSG/OSGLine.h>
#include "core/math/pose.h"

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
            int eventState = -1;
            Touch(int k = -1);
        };

    private:
        int fd;
        mtdev dev;
        VRUpdateCbPtr updatePtr;
        VRCameraWeakPtr cam;
        VRWindowWeakPtr window;
        Line ray;

        static vector<string> devices;
        static vector<string> deviceIDs;

        void multFull(Matrix _matrix, const Pnt3f &pntIn, Pnt3f  &pntOut);
        bool calcViewRay(VRCameraPtr pcam, VRViewPtr view, Line &line, float x, float y, int W, int H);

        string device;
        string input;
        string devID;
        map<int, Touch> fingers;
        int currentFingerID = 0;

        float clamp(float v, float m1, float m2);
        bool rescale(float& v, float m1, float m2);

    public:
        VRMultiTouch();
        ~VRMultiTouch();

        static VRMultiTouchPtr create();
        VRMultiTouchPtr ptr();

        static vector<string> getDevices();
        string getDevice();
        string getInput();
        void setDevice(string dev);

        void updateDevice();
        void connectDevice();
        void disconnectDevice();
        void clearSignals();

        //3d object to emulate a hand in VRSpace
        void updatePosition(int x, int y);
        void mouse(int button, int state, int x, int y);

        Line getRay();
        void setCamera(VRCameraPtr cam);
        void setWindow(VRWindowPtr win);
};

OSG_END_NAMESPACE;

#endif // VRMULTITOUCH_H_INCLUDED
