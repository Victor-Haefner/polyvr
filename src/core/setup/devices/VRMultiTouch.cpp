#include "VRMultiTouch.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRGtkWindow.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/VRCamera.h"
#include "core/objects/OSGCamera.h"
#include "core/tools/VRAnalyticGeometry.h"
#include "VRSignal.h"
#include <GL/glut.h>
#include <OpenSG/OSGPerspectiveCamera.h>

#include <mtdev.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include <sys/types.h>
#include <sys/wait.h>

typedef __u64 mstime_t;

using namespace OSG;


VRMultiTouch::Touch::Touch(int k) : key(k) {}

VRMultiTouch::VRMultiTouch() : VRDevice("multitouch") {
    addBeacon();
    fingers[0] = Touch(0);
    fingers[0].beaconID = 0;

    store("device", &device);
    store("input", &input);

    regStorageSetupFkt( VRStorageCb::create("connect mt device", bind(&VRMultiTouch::setup, this, _1)) );
}

VRMultiTouch::~VRMultiTouch() {
    disconnectDevice();
}

VRMultiTouchPtr VRMultiTouch::create() {
    auto m = VRMultiTouchPtr(new VRMultiTouch());
    m->initIntersect(m);
    m->clearSignals();
    return m;
}

VRMultiTouchPtr VRMultiTouch::ptr() { return static_pointer_cast<VRMultiTouch>( shared_from_this() ); }

void VRMultiTouch::setup(VRStorageContextPtr context) {
    connectDevice();
}

#define die(e) do { fprintf(stderr, "%s\n", e); exit(EXIT_FAILURE); } while (0);

#include <cstdarg>

template <typename ...Args>
string execCmd(Args&&... args) {
    int link[2];
    if (pipe(link) == -1) die("pipe");
    pid_t pid = fork();
    if (pid == -1) die("fork");

    if (pid == 0) {
        dup2 (link[1], STDOUT_FILENO);
        close(link[0]);
        close(link[1]);
        execl( args..., (char*)0 );
        die("execl");
        return 0;
    }

    string result;
    close(link[1]);
    char foo[32768];
    size_t N = read(link[0], foo, sizeof(foo));
    result = string(foo, N);
    int status;
    wait(&status);
    return result;
}

vector<string> VRMultiTouch::devices = vector<string>();
vector<string> VRMultiTouch::deviceIDs = vector<string>();

vector<string> VRMultiTouch::getDevices() {
    devices = splitString(execCmd("/usr/bin/xinput", "xinput", "list", "--name-only"), '\n');
    deviceIDs = splitString(execCmd("/usr/bin/xinput", "xinput", "list", "--id-only"), '\n');
    return devices;
}

string VRMultiTouch::getDevice() { return device; }
string VRMultiTouch::getInput() { return input; }

void VRMultiTouch::setDevice(string devName) {
    disconnectDevice();
    device = devName;
    connectDevice();
}

/*static void print_event(const struct input_event *ev) {
	static const mstime_t ms = 1000;
	static int slot;
	mstime_t evtime = ev->time.tv_usec / ms + ev->time.tv_sec * ms;
	if (ev->type == EV_ABS && ev->code == ABS_MT_SLOT) slot = ev->value;
	fprintf(stderr, "%012llx %02d %01d %04x %d\n", evtime, slot, ev->type, ev->code, ev->value);
}*/

void VRMultiTouch::updateDevice() {
    if (!visual) showVisual(true);

	struct input_event ev;
	string txt;
	//while (!mtdev_idle(&dev, fd, 15000)) { // while the device has not been inactive for fifteen seconds */
    while (mtdev_get(&dev, fd, &ev, 1) > 0) {
        // Recorded event types (ev.type):
        // 0: Unsure what it does, but seems uninteresting, ev.code and ev.value are always 0
        // 1: Only signals start (ev.value==1) and end (ev.value==0) of touch events
        // 3: This is the interesting part. Contains all touch and multitouch events.
        //    ev.code contains the type of touch event

        auto& finger = fingers[currentFingerID];

        if (ev.type == 3) {
            switch (ev.code) {
            case 0:
                // only for single touch, 53 has same info and more
                txt = "ABS_X";
                break;
            case 1:
                // only for single touch, 54 has same info and more
                txt = " ABS_Y";
                break;
            case 47:
                txt = "ABS_MT_SLOT";
                if (!fingers.count(ev.value)) this->addFinger(ev.value);
                currentFingerID = ev.value;

                //cout << "SLOT: " << ev.value;
                break;
            case 48:
                txt = "ABS_MT_TOUCH_MAJOR";
                break;
            case 49:
                txt = "ABS_MT_TOUCH_MINOR";
                break;
            case 52:
                txt = "ABS_MT_ORIENTATION";
                break;
            case 53:
                txt = " ABS_MT_POSITION_X";
                if (currentFingerID == -1) return;
                finger.pos[0] = ev.value;
                updatePosition(finger.pos[0], finger.pos[1]);

                if ( finger.eventState >= 0) finger.eventState++;
                if ( finger.eventState == 2) {
                    mouse(currentFingerID,finger.pos[2], finger.pos[0], finger.pos[1]);
                    if (visual && visual->isVisible()) {
                        auto bPose = getBeacon(finger.beaconID)->getPose();
                        Vec3d p = bPose->pos() + bPose->dir()*1;
                        visual->setCircle(currentFingerID, p, Vec3d(0,0,1), 0.1, Color3f(0,0.3,1));
                    }
                }
                break;
            case 54:
                txt = "  ABS_MT_POSITION_Y";
                if (currentFingerID == -1) return;
                finger.pos[1] = ev.value;
                updatePosition(finger.pos[0], finger.pos[1]);

                if ( finger.eventState >= 0 ) finger.eventState++;
                if ( finger.eventState == 2 ) {
                    mouse(currentFingerID,finger.pos[2], finger.pos[0], finger.pos[1]);
                    if (visual && visual->isVisible())  {
                        auto bPose = getBeacon(finger.beaconID)->getPose();
                        Vec3d p = bPose->pos() + bPose->dir()*1;
                        visual->setCircle(currentFingerID, p, Vec3d(0,0,1), 0.1, Color3f(0,0.3,1));
                    }
                }
                break;
            case 57:
                txt = "ABS_MT_TRACKING_ID";
                if (ev.value == -1) { // Finger is "released" as in not present on the touch surface anymore
                    finger.pos[2] = 0;
                    mouse(currentFingerID, finger.pos[2], finger.pos[0], finger.pos[1]);
                    if (visual && visual->isVisible()) visual->setCircle(currentFingerID, Vec3d(), Vec3d(0,1,0), 0, Color3f(0,0.3,1));
                } else { // New touch event.
                    finger.pos[2] = 1;
                    finger.eventState = 0;
                }
                break;
            default:
                txt = "ERROR: UNKNOWN EVENT CODE";
            }


            if (ev.code == 53 || ev.code == 54 || ev.code == 57 ) {
                //cout << " " << txt << " : " << ev.value << endl;
                for (auto f : fingers) {
                    if (f.second.pos[2] == 0) continue;
                    //cout << " Finger: ID " << f.second.key << " pos " << f.second.pos << endl;
                }
            }
        }
    }
	//}
}

void VRMultiTouch::showVisual(bool b) {
    if (!visual) {
        visual = VRAnalyticGeometry::create("touch_visuals");
        getBeacon()->getParent()->addChild(visual);
    }

    visual->setVisible(b);
}

void VRMultiTouch::disconnectDevice() {
    if (devID != "") {
        execCmd("/usr/bin/xinput", "xinput", "enable", devID.c_str());
        mtdev_close(&dev);
        ioctl(fd, EVIOCGRAB, 0);
        close(fd);
    }
}

void VRMultiTouch::connectDevice() {
    devID = "";
    vector<int> IDs;
    for (uint i=0; i<devices.size(); i++) {
        if (devices[i] == device) IDs.push_back( toInt(deviceIDs[i]) );
    }
    if (IDs.size() == 0) return;

    int ID = IDs[0];

    if (IDs.size() > 1) { // more than one ID
        for (auto id : IDs) {
            devID = toString(id);
            string props = execCmd("/usr/bin/xinput", "xinput", "list-props", devID.c_str());
            auto mtPos = props.find("Abs MT Position");
            if (mtPos != string::npos) ID = id;
        }
    }

    devID = toString(ID);

    string props = execCmd("/usr/bin/xinput", "xinput", "list-props", devID.c_str());
    auto eventPos = props.find("/dev/input/event");
    input = "";
    for (; props[eventPos+1] != '\n'; eventPos++) input += props[eventPos];
    execCmd("/usr/bin/xinput", "xinput", "disable", devID.c_str());

    fd = open(input.c_str(), O_RDONLY | O_NONBLOCK);
	if (fd < 0) { cout << "VRMultiTouch::connectDevice Error: could not open device " << input << endl; return; }
	if (ioctl(fd, EVIOCGRAB, 1)) { cout << "VRMultiTouch::connectDevice Error: could not grab device " << input << endl; return; }

	int ret = mtdev_open(&dev, fd);
	if (ret) { cout << "VRMultiTouch::connectDevice Error: could not open device: " << input << endl; return; }

    updatePtr = VRUpdateCb::create( "MultiTouch_update", bind(&VRMultiTouch::updateDevice, this) );
    VRSceneManager::get()->addUpdateFkt(updatePtr);
    cout << "VRMultiTouch::connectDevice successfully connected to device " << devID << " at event " << input << endl;
}

void VRMultiTouch::clearSignals() {
    VRDevice::clearSignals();
    addSlider(5);
    addSlider(6);

    newSignal( 0, 0)->add( getDrop() );
    newSignal( 0, 1)->add( addDrag( getBeacon() ) );
}

void VRMultiTouch::multFull(Matrix _matrix, const Pnt3f &pntIn, Pnt3f  &pntOut) {
    float w = _matrix[0][3] * pntIn[0] +
                  _matrix[1][3] * pntIn[1] +
                  _matrix[2][3] * pntIn[2] +
                  _matrix[3][3];


    if(w == TypeTraits<float>::getZeroElement())
    {
        cout << "\nWARNING: w = " << _matrix[0][3] * pntIn[0] << " " << _matrix[1][3] * pntIn[1] << " " << _matrix[2][3] * pntIn[2];


        pntOut.setValues(
            (_matrix[0][0] * pntIn[0] +
             _matrix[1][0] * pntIn[1] +
             _matrix[2][0] * pntIn[2] +
             _matrix[3][0]             ),
            (_matrix[0][1] * pntIn[0] +
             _matrix[1][1] * pntIn[1] +
             _matrix[2][1] * pntIn[2] +
             _matrix[3][1]             ),
            (_matrix[0][2] * pntIn[0] +
             _matrix[1][2] * pntIn[1] +
             _matrix[2][2] * pntIn[2] +
             _matrix[3][2]             ) );
    }
    else
    {
        w = TypeTraits<float>::getOneElement() / w;

        pntOut.setValues(
            (_matrix[0][0] * pntIn[0] +
             _matrix[1][0] * pntIn[1] +
             _matrix[2][0] * pntIn[2] +
             _matrix[3][0]             ) * w,
            (_matrix[0][1] * pntIn[0] +
             _matrix[1][1] * pntIn[1] +
             _matrix[2][1] * pntIn[2] +
             _matrix[3][1]             ) * w,
            (_matrix[0][2] * pntIn[0] +
             _matrix[1][2] * pntIn[1] +
             _matrix[2][2] * pntIn[2] +
             _matrix[3][2]             ) * w );
    }
}

/**
* Attempts to add a new finger with the given id
* return: True, if finger has been created. False, if finger with given id already exists.
*/
bool VRMultiTouch::addFinger(int fingerID) {
    if (!fingers.count(fingerID)) {
        fingers[fingerID] = Touch(fingerID);
        fingers[fingerID].beaconID = this->addBeacon();

        return true;
    }

    return true;
}

/**
* TODO: Duplicate code with VRMouse. Push Method to super class?
*/
bool VRMultiTouch::calcViewRay(VRCameraPtr cam, VRViewPtr view, Line &line, float x, float y) {
    if (!cam) return false;

    int w, h;
    // TODO: returns 320x240 instead of actual parameters for some reason.
    w = view->getViewportL()->calcPixelWidth();
    h = view->getViewportL()->calcPixelHeight();

//    cout << "VRMultiTouch::calcViewRay. w,h: " << Vec2i(w,h) << endl;

    if (w <= 0 || h <= 0) return false;

    Matrix proj, projtrans;
    if (view && view->getCameraDecoratorLeft()) {
        auto c = view->getCameraDecoratorLeft();
        c->getProjection(proj, w, h);
        c->getProjectionTranslation(projtrans, w, h);
    } else { // TODO: send window parameters from slaves back to master and update view size!
        //////////////////////////////////////
        //Temporary Hack to get the right aspect ratio
        w = 16;
        h = 9;
        //////////////////////////////////////
        cam->getCam()->cam->getProjection(proj, w, h);
        cam->getCam()->cam->getProjectionTranslation(projtrans, w, h);
    }

    Matrix wctocc;
    wctocc.mult(proj);
    wctocc.mult(projtrans);

    Matrix cctowc;
    cctowc.invertFrom(wctocc);


    Pnt3f from, at;
    multFull(cctowc, Pnt3f(x, y, 0), from); // -1
    multFull(cctowc, Pnt3f(x, y, 1), at ); // 0.1
    if (view && view->getCameraDecoratorLeft()) from += Vec3f(view->getProjectionUser());

//    cout << "VRMultiTouch::calcViewRay cam: " << cam->getName() << " from: " << from << endl;

    Vec3f dir = at - from;
    dir.normalize();

    if (cam->getType() == 1) { // hack for ortho cam, TODO: not working :(
        from[2] = 0;
    }

    line.setValue(from, dir);
    return true;
}


bool VRMultiTouch::rescale(float& v, float m1, float m2) {
    // Rescale v to window coordinates ([-1;1]) of given bounds m1, m2
    v = (v-m1)/(m2-m1) *2 -1;
    // check if v is within bounds
    return (v >= -1 && v <= 1);
}

//3d object to emulate a hand in VRSpace
void VRMultiTouch::updatePosition(int x, int y) {

    auto cam = this->cam.lock(); // TODO: is this obsolete after adding "cam = view->getCamera();" in if(inside)?
    auto win = window.lock();
    if (!cam) { cout << "Warning: VRMultiTouch::updatePosition, no camera defined!" << endl; return; }
    if (!win) { cout << "Warning: VRMultiTouch::updatePosition, no window defined!" << endl; return; }


    for (auto view : win->getViews()) {
        // Find the view in which the touch event is currently positioned

        // Get position coords of the views from [0;1] to [-1;1]
        Vec4d box = view->getPosition()*2 - Vec4d(1,1,1,1);

        // switch y position coords, so that -1 is on the lower bound, not the upper
        auto tmp = box[1];
        box[1] = -box[3];
        box[3] = -tmp;

        // Normalize input coords to [-1;1]
        // Needs to be in loop, since rx, ry are modified by rescale
        float rx, ry;
        rx =  (x/float(maxX) - 0.5 )*2;
        ry = -(y/float(maxY) - 0.5 )*2;

        bool inside = rescale(rx, box[0], box[2]) && rescale(ry, box[1], box[3]);
//        cout << "X: " << Vec2f(box[0], box[2]) << ", Y: " << Vec2f(box[1], box[3]) << ". rx,ry: " << Vec2f(rx,ry) << " --> " << inside << endl;

        if (inside) {

            cam = view->getCamera();

            calcViewRay(cam, view, ray, rx,ry);
            auto p = Pose::create(Vec3d(ray.getPosition()), Vec3d(ray.getDirection()));
            p->makeUpOrthogonal();

            auto beacon = getBeacon(this->fingers[currentFingerID].beaconID);
            beacon->setPose(p);
            if (!beacon->hasAncestor(cam)) cam->getSetupNode()->addChild(beacon);

//            cout << "  Update MT x y (" << Vec2i(x,y) << "), rx ry (" << Vec2f(rx,ry) << "), w h (" << Vec2i(w,h) << ") dir " << getBeacon()->getDir() << endl;
            break;
        }
    }
}

/**
* Is invoked whenever a touch event happens.
* @param button: id of the "button", mostly meaning which finger is touching. First event has id=0,
* second consecutive event has id=1 and so on. Releasing a finger frees its id up for further events.
* @param state: state of the event. 1 = press, 0 = release
* @param x: x coordinate of the touch device, not necessarily same resolution as screen
* @param y: y coordinate of the touch device, not necessarily same resolution as screen
*/
void VRMultiTouch::mouse(int button, int state, int x, int y) {


    //cout << "VRMultiTouch::mouse  Button: " << button << ", State: " << state << ", X:Y: " << x << ":" << y << endl;

    updatePosition(x,y);
    change_button(button, state);
    fingers[this->currentFingerID].eventState = -1;
}

void VRMultiTouch::setCamera(VRCameraPtr cam) { this->cam = cam; }
void VRMultiTouch::setWindow(VRWindowPtr win) { this->window = win; }

Line VRMultiTouch::getRay() { return ray; }


