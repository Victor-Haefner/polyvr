#ifndef VRMULTITOUCH_H_INCLUDED
#define VRMULTITOUCH_H_INCLUDED

#include "core/setup/windows/VRView.h"
#include "VRDevice.h"
#include <OpenSG/OSGLine.h>
#include "core/math/pose.h"
#include "core/tools/VRToolsFwd.h"

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
            int beaconID;
        };

    private:
        // maximum raw coordinates of MT Device
        // Min and max values from manual experiments:
        // X: min ~70, max ~28560
        // Y: min ~80, max ~16020
        const static int maxX = 28630;
        const static int maxY = 16100;

        int fd;
        mtdev dev;
        VRUpdateCbPtr updatePtr;
        VRCameraWeakPtr cam;
        VRWindowWeakPtr window;
        VRAnalyticGeometryPtr visual;
        Line ray;

        static vector<string> devices;
        static vector<string> deviceIDs;

        void multFull(Matrix _matrix, const Pnt3f &pntIn, Pnt3f  &pntOut);
        bool calcViewRay(VRCameraPtr pcam, VRViewPtr view, Line &line, float x, float y);

        string device;
        string input;
        string devID;
        map<int, Touch> fingers;
        int currentFingerID = 0;

        float clamp(float v, float m1, float m2);
        bool rescale(float& v, float m1, float m2);

        bool addFinger(int fingerID);

    public:
        VRMultiTouch();
        ~VRMultiTouch();

        static VRMultiTouchPtr create();
        VRMultiTouchPtr ptr();

        void setup(VRStorageContextPtr context);

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

        void showVisual(bool b);
};

OSG_END_NAMESPACE;

#endif // VRMULTITOUCH_H_INCLUDED
