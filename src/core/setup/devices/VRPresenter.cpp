#include "VRPresenter.h"
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

//#include <mtdev.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <linux/input.h>

typedef __u64 mstime_t;

using namespace OSG;


VRPresenter::Touch::Touch(int k) : key(k) {}

VRPresenter::VRPresenter() : VRFlystick() {
    addBeacon();
    fingers[0] = Touch(0);
    fingers[0].beaconID = 0;

    store("device", &device);
    store("input", &input);

    regStorageSetupFkt( VRStorageCb::create("connect mt device", bind(&VRPresenter::setup, this, _1)) );
}

VRPresenter::~VRPresenter() {
    disconnectDevice();
}

VRPresenterPtr VRPresenter::create() {
    auto m = VRPresenterPtr(new VRPresenter());
    m->initIntersect(m);
    m->clearSignals();
    return m;
}

VRPresenterPtr VRPresenter::ptr() { return static_pointer_cast<VRPresenter>( shared_from_this() ); }

void VRPresenter::setup(VRStorageContextPtr context) {
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

vector<string> VRPresenter::devices = vector<string>();
vector<string> VRPresenter::deviceIDs = vector<string>();

vector<string> VRPresenter::getDevices() {
    devices = splitString(execCmd("/usr/bin/xinput", "xinput", "list", "--name-only"), '\n');
    deviceIDs = splitString(execCmd("/usr/bin/xinput", "xinput", "list", "--id-only"), '\n');
    return devices;
}

string VRPresenter::getDevice() { return device; }
string VRPresenter::getInput() { return input; }

void VRPresenter::setDevice(string devName) {
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


void VRPresenter::updateDevice() {
    //cout << "VRPresenter::updateDevice" << endl;
    struct input_event ev;
    static int oldButtonState = 0;
    int ret = 0;

    vector<int>   buttons = {0,0,0,0};
    vector<float> sliders = {0,0};

    while ((ret = read(fd, &ev, sizeof(ev))) == sizeof(ev)) {
	    if (ev.type == EV_KEY) {
		int b = -1;
		if (ev.code == 104) // left
		    b = 0;
		else if (ev.code == 109) // right
		    b = 1;
		else if ((ev.code == 1) || (ev.code == 63) || ev.code == 425) // esc/F5
		    b = 2;
		else if (ev.code == 52 || ev.code == 431) // blank
		    b = 3;
		else if (ev.code == 114) // -
		    b = 4;
		else if (ev.code == 115) // +
		    b = 5;
		else if (ev.code >= BTN_0 && ev.code <= BTN_9)
		{
		    b = ev.code - BTN_0;
		}
		else if (ev.code >= BTN_TRIGGER && ev.code <= BTN_DEAD)
		{
		    b = ev.code - BTN_TRIGGER;
		}
		else if (ev.code >= BTN_A && ev.code <= BTN_THUMBR)
		{
		    b = ev.code - BTN_A;
		}
		else if (ev.code >= BTN_LEFT && ev.code <= BTN_TASK)
		{
		    b = ev.code - BTN_LEFT;
		}
		else
		{
		    std::cerr << "unknown evdev code: " << ev.code << std::endl;
		}
		if (ev.value > 0) oldButtonState |= 1 << b;
		else oldButtonState &= ~(1 << b);
		bool pressed = (oldButtonState>0);

	        cout << " VRPresenter::updateDevice key: " << b << ", state: " << pressed << endl;
		if (b == 1) buttons[0] = pressed;
		if (b == 0) sliders[1] = 1.0 * pressed;
		if (b == 2) sliders[0] = -1.0 * pressed;
		if (b == 3) sliders[0] =  1.0 * pressed;
		VRFlystick::update(buttons);
		VRFlystick::update(sliders);
	    }
    }

}

void VRPresenter::showVisual(bool b) {
    if (!visual) {
        visual = VRAnalyticGeometry::create("touch_visuals");
        getBeacon()->getParent()->addChild(visual);
    }

    visual->setVisible(b);
}

void VRPresenter::disconnectDevice() {
    //ioctl(fd, EVIOCGRAB, 0);
    close(fd);
}

void VRPresenter::connectDevice() {
    cout << "VRPresenter::connectDevice" << endl;
    input = "/dev/input/presenter";
    fd = open(input.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) { cout << "VRPresenter::connectDevice Error: could not open device " << input << endl; return; }
    //if (ioctl(fd, EVIOCGRAB, 1)) { cout << "VRPresenter::connectDevice Error: could not grab device " << input << endl; return; }

    updatePtr = VRUpdateCb::create( "Presenter_update", bind(&VRPresenter::updateDevice, this) );
    VRSceneManager::get()->addUpdateFkt(updatePtr);
    cout << "VRPresenter::connectDevice successfully connected to device " << devID << " at event " << input << endl;
}

void VRPresenter::clearSignals() {
    VRDevice::clearSignals();

    newSignal( 0, 0)->add( getDrop() );
    newSignal( 0, 1)->add( addDrag( getBeacon() ) );
}

void VRPresenter::multFull(Matrix _matrix, const Pnt3f &pntIn, Pnt3f  &pntOut) {
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
bool VRPresenter::addFinger(int fingerID) {
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
bool VRPresenter::calcViewRay(VRCameraPtr cam, VRViewPtr view, Line &line, float x, float y) {
    if (!cam) return false;

    int w, h;
    // TODO: returns 320x240 instead of actual parameters for some reason.
    w = view->getViewportL()->calcPixelWidth();
    h = view->getViewportL()->calcPixelHeight();

//    cout << "VRPresenter::calcViewRay. w,h: " << Vec2i(w,h) << endl;

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

//    cout << "VRPresenter::calcViewRay cam: " << cam->getName() << " from: " << from << endl;

    Vec3f dir = at - from;
    dir.normalize();

    if (cam->getType() == 1) { // hack for ortho cam, TODO: not working :(
        from[2] = 0;
    }

    line.setValue(from, dir);
    return true;
}


bool VRPresenter::rescale(float& v, float m1, float m2) {
    // Rescale v to window coordinates ([-1;1]) of given bounds m1, m2
    v = (v-m1)/(m2-m1) *2 -1;
    // check if v is within bounds
    return (v >= -1 && v <= 1);
}

//3d object to emulate a hand in VRSpace
void VRPresenter::updatePosition(int x, int y) {

    auto cam = this->cam.lock(); // TODO: is this obsolete after adding "cam = view->getCamera();" in if(inside)?
    auto win = window.lock();
    if (!cam) { cout << "Warning: VRPresenter::updatePosition, no camera defined!" << endl; return; }
    if (!win) { cout << "Warning: VRPresenter::updatePosition, no window defined!" << endl; return; }


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
void VRPresenter::mouse(int button, int state, int x, int y) {


    //cout << "VRPresenter::mouse  Button: " << button << ", State: " << state << ", X:Y: " << x << ":" << y << endl;

    updatePosition(x,y);
    change_button(button, state);
    fingers[this->currentFingerID].eventState = -1;
}

void VRPresenter::setCamera(VRCameraPtr cam) { this->cam = cam; }
void VRPresenter::setWindow(VRWindowPtr win) { this->window = win; }

Line VRPresenter::getRay() { return ray; }


