#include "VRMultiTouch.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRGtkWindow.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/VRCamera.h"
#include "core/objects/OSGCamera.h"
#include "VRSignal.h"
#include <GL/glut.h>
#include <OpenSG/OSGPerspectiveCamera.h>

#include <mtdev.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>

typedef __u64 mstime_t;

OSG_BEGIN_NAMESPACE;
using namespace std;

VRMultiTouch::Touch::Touch(int k) : key(k) {}

VRMultiTouch::VRMultiTouch() : VRDevice("multitouch") { connectDevice(); }

VRMultiTouch::~VRMultiTouch() {
    mtdev_close(&dev);
	ioctl(fd, EVIOCGRAB, 0);
	close(fd);
}

VRMultiTouchPtr VRMultiTouch::create() {
    auto m = VRMultiTouchPtr(new VRMultiTouch());
    m->initIntersect(m);
    m->clearSignals();
    return m;
}

VRMultiTouchPtr VRMultiTouch::ptr() { return static_pointer_cast<VRMultiTouch>( shared_from_this() ); }

static void print_event(const struct input_event *ev) {
	static const mstime_t ms = 1000;
	static int slot;
	mstime_t evtime = ev->time.tv_usec / ms + ev->time.tv_sec * ms;
	if (ev->type == EV_ABS && ev->code == ABS_MT_SLOT) slot = ev->value;
	fprintf(stderr, "%012llx %02d %01d %04x %d\n", evtime, slot, ev->type, ev->code, ev->value);
}

void VRMultiTouch::updateDevice() {
	struct input_event ev;
	string txt;
	//while (!mtdev_idle(&dev, fd, 15000)) { // while the device has not been inactive for fifteen seconds */
		while (mtdev_get(&dev, fd, &ev, 1) > 0) {
            switch(ev.type) {
            case 0:
                // seems uninteresting, ev.code and ev.value always 0
//                fprintf(stderr, "%01d : %d, %d\n", ev.type, ev.code, ev.value);
                break;
            case 1:
                fprintf(stderr, "Touch %s\n", ev.value == 1? "start": "end");
                break;
            case 3:
//                fprintf(stderr, "    %01d : %d, %d\n", ev.type, ev.code, ev.value);
                break;
            default:
//                fprintf(stderr, "Unmanaged mt event type: %02d\n", ev.type);
                break;
            }
            if (ev.type == 3) {
                switch (ev.code) {
                case 0:
                    txt = "ABS_X";
                    break;
                case 1:
                    txt = " ABS_Y";
                    break;
                case 47:
                    txt = "ABS_MT_SLOT";
                    if (!fingers.count(ev.value)) fingers[ev.value] = Touch(ev.value);
                    currentTouchID = ev.value;
                    break;
                case 48:
                    txt = "ABS_MT_TOUCH_MAJOR";
                    break;
                case 49:
                    txt = "ABS_MT_TOUCH_MINOR";
                    break;
                case 52:
                    txt = "52";
                    break;
                case 53:
                    txt = "ABS_MT_POSITION_X";
                    if (currentTouchID == -1) return;
                    fingers[currentTouchID].pos[0] = ev.value;
                    updatePosition(fingers[currentTouchID].pos[0], fingers[currentTouchID].pos[1]);
                    break;
                case 54:
                    txt = " ABS_MT_POSITION_Y";
                    if (currentTouchID == -1) return;
                    fingers[currentTouchID].pos[1] = ev.value;
                    updatePosition(fingers[currentTouchID].pos[0], fingers[currentTouchID].pos[1]);
                    break;
                case 57:
                    txt = "ABS_MT_TRACKING_ID";
                    if (currentTouchID == -1) return;
                    fingers[currentTouchID].pos[2] = (ev.value == -1) ? 0 : 1;
                    mouse(0,fingers[currentTouchID].pos[2], fingers[currentTouchID].pos[0], fingers[currentTouchID].pos[1]);
                    break;
                default:
                    txt = "err";
                    cout << "UNKNOWN CODE " << ev.code << " : " << ev.value << endl;
                }
                if (ev.code == 53 || ev.code == 54 || ev.code == 57) {
                    //cout << txt << " : " << ev.value << endl;
                    for (auto f : fingers) {
                        if (f.second.pos[2] == 0) continue;
                        cout << " finger: ID " << f.second.key << " pos " << f.second.pos << endl;
                    }
                }
                if (ev.code == 52) {
                    cout << ev.code << " : " << ev.value << endl;
                }
            }
		}
	//}
}

void VRMultiTouch::connectDevice() {
    fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
	if (fd < 0) { cout << "VRMultiTouch::connectDevice Error: could not open device\n"; return; }
	if (ioctl(fd, EVIOCGRAB, 1)) { cout << "VRMultiTouch::connectDevice Error: could not grab device\n"; return; }

	int ret = mtdev_open(&dev, fd);
	if (ret) { cout << "VRMultiTouch::connectDevice Error: could not open device: " << ret << endl; return; }

    updatePtr = VRUpdateCb::create( "MultiTouch_update", boost::bind(&VRMultiTouch::updateDevice, this) );
    VRSceneManager::get()->addUpdateFkt(updatePtr);
}

void VRMultiTouch::clearSignals() {
    VRDevice::clearSignals();
    addSlider(5);
    addSlider(6);

    addSignal( 0, 0)->add( getDrop() );
    addSignal( 0, 1)->add( addDrag( getBeacon() ) );
}

void VRMultiTouch::multFull(Matrix _matrix, const Pnt3f &pntIn, Pnt3f  &pntOut) {
    float w = _matrix[0][3] * pntIn[0] +
                  _matrix[1][3] * pntIn[1] +
                  _matrix[2][3] * pntIn[2] +
                  _matrix[3][3];

    /*if (w <1) {
        fstream file("dump.txt", fstream::out | fstream::app);
        file << w << endl;
        file.close();
    }*/

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

bool VRMultiTouch::calcViewRay(VRCameraPtr cam, Line &line, float x, float y, int W, int H) {
    if (!cam) return false;
    if (W <= 0 || H <= 0) return false;

    Matrix proj, projtrans, view;

    cam->getCam()->cam->getProjection(proj, W, H);
    cam->getCam()->cam->getProjectionTranslation(projtrans, W, H);

    Matrix wctocc;
    wctocc.mult(proj);
    wctocc.mult(projtrans);

    Matrix cctowc;
    cctowc.invertFrom(wctocc);


    Pnt3f from, at;
    multFull(cctowc, Pnt3f(x, y, 0), from); // -1
    multFull(cctowc, Pnt3f(x, y, 1), at ); // 0.1

    Vec3f dir = at - from;
    dir.normalize();

    line.setValue(from, dir);
    return true;
}

//3d object to emulate a hand in VRSpace
void VRMultiTouch::updatePosition(int x, int y) {
    auto cam = this->cam.lock();
    if (!cam) return;
    auto v = view.lock();
    if (!v) return;

    int w, h;
    w = v->getViewport()->calcPixelWidth();
    h = v->getViewport()->calcPixelHeight();

    float rx, ry;
    //v->getViewport()->calcNormalizedCoordinates(rx, ry, x, y);
    rx =  (x/28430.0 - 0.5 )*2; // 65535.0
    ry = -(y/16000.0 - 0.5 )*2;

    cout << " multitouch x y " << Vec2i(x,y) << " rx ry " << Vec2f(rx,ry) << " w h " << Vec2i(w,h) << endl;

    //cam->getCam()->calcViewRay(ray,x,y,*v->getViewport());
    calcViewRay(cam, ray, rx,ry,w,h);
    editBeacon()->setDir(Vec3d(ray.getDirection()));
}

void VRMultiTouch::mouse(int button, int state, int x, int y) {
    float _x, _y;
    auto sv = view.lock();
    if (!sv) return;

    ViewportRecPtr v = sv->getViewport();
    v->calcNormalizedCoordinates(_x, _y, x, y);
    change_slider(5,_x);
    change_slider(6,_y);

    updatePosition(x,y);
    if (state) change_button(button,false);
    else change_button(button,true);
}

void VRMultiTouch::motion(int x, int y) {
    auto sv = view.lock();
    if (!sv) return;

    float _x, _y;
    ViewportRecPtr v = sv->getViewport();
    v->calcNormalizedCoordinates(_x, _y, x, y);
    change_slider(5,_x);
    change_slider(6,_y);

    updatePosition(x,y);
}

void VRMultiTouch::setCamera(VRCameraPtr cam) { this->cam = cam; }
void VRMultiTouch::setViewport(VRViewPtr view) { this->view = view; }

Line VRMultiTouch::getRay() { return ray; }

void VRMultiTouch::save(xmlpp::Element* e) {
    VRDevice::save(e);
}

void VRMultiTouch::load(xmlpp::Element* e) {
    VRDevice::load(e);
}

OSG_END_NAMESPACE;
