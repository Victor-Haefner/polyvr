#include "VRMouse.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRWindow.h"
#ifndef WITHOUT_GTK
#include "core/setup/windows/VRGtkWindow.h"
#endif
#include "core/objects/VRCamera.h"
#include "core/objects/OSGCamera.h"
#include "VRSignal.h"

#include <OpenSG/OSGPerspectiveCamera.h>

using namespace OSG;


VRMouse::VRMouse() : VRDevice("mouse") { addBeacon(); }
VRMouse::~VRMouse() {}

VRMousePtr VRMouse::create() {
    auto m = VRMousePtr(new VRMouse());
    m->on_to_edge = VRSignal::create(m);
    m->on_from_edge = VRSignal::create(m);
    m->initIntersect(m);
    m->clearSignals();
    return m;
}

VRMousePtr VRMouse::ptr() { return static_pointer_cast<VRMouse>( shared_from_this() ); }

void VRMouse::setCursor(string c) {
    auto s = VRSetup::getCurrent();
    for (auto w : s->getWindows()) {
        if (!w.second->hasType(2)) continue; // not a gtk window
#ifndef WITHOUT_GTK
        auto win = dynamic_pointer_cast<VRGtkWindow>(w.second);
        win->setCursor(c);
#endif
    }
}

void VRMouse::clearSignals() {
    VRDevice::clearSignals();
    addSlider(5);
    addSlider(6);

    newSignal( 0, 0)->add( getDrop() );
    newSignal( 0, 1)->add( addDrag( getBeacon() ) );

    if (on_to_edge) on_to_edge->clear();
    if (on_from_edge) on_from_edge->clear();
}

void VRMouse::multFull(Matrix _matrix, const Pnt3f &pntIn, Pnt3f  &pntOut) {
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

/**
* TODO: Duplicate code with VRMultitouch. Push Method to super class or use Interface?
*/
bool VRMouse::calcViewRay(VRCameraPtr cam, Line &line, float x, float y, int W, int H) {
    if (!cam) return false;
    if(W <= 0 || H <= 0) return false;

    auto v = view.lock();
    Matrix proj, projtrans;

    if (v && v->getCameraDecoratorLeft()) {
        auto c = v->getCameraDecoratorLeft();
        c->getProjection(proj, W, H);
        c->getProjectionTranslation(projtrans, W, H);
    } else {
        cam->getCam()->cam->getProjection(proj, W, H);
        cam->getCam()->cam->getProjectionTranslation(projtrans, W, H);
    }

    Matrix wctocc;
    wctocc.mult(proj);
    wctocc.mult(projtrans);

    Matrix cctowc;
    cctowc.invertFrom(wctocc);


    Pnt3f from, at;
    multFull(cctowc, Pnt3f(x, y, 0), from); // -1
    multFull(cctowc, Pnt3f(x, y, 1), at ); // 0.1
    if (v && v->getCameraDecoratorLeft()) from += Vec3f(v->getProjectionUser());

    Vec3f dir = at - from;
    dir.normalize();

    if (cam->getType() == 1) { // hack for ortho cam, TODO: not working :(
        from[2] = 0;
    }

    //cout << "VRMouse::calcViewRay xy " << Vec2i(x,y) << " from " << from << " at " << at << " dir " << dir << endl;
    line.setValue(from, dir);
    return true;
}

VRSignalPtr VRMouse::getToEdgeSignal() { return on_to_edge; }
VRSignalPtr VRMouse::getFromEdgeSignal() { return on_from_edge; }

//3d object to emulate a hand in VRSpace
void VRMouse::updatePosition(int x, int y) {
    auto cam = this->cam.lock();
    if (!cam) return;

    auto v = view.lock();
    if (!v) return;

    auto altCam = v->getCamera();
    if (altCam && cam != altCam) {

        cout << "VRMouse::updatePosition. Changed cam to: " << altCam->getName() << endl;

        setCamera(altCam);

        auto beacon = getBeacon();
        cout << "VRMouse::updatePosition. Beacon: " << beacon->getName() << endl;
        if (!beacon->hasAncestor(altCam)) {
                cout << "change beacon's ancestor to " << altCam->getSetupNode()->getName() << endl;
                altCam->getSetupNode()->addChild(beacon);
        }
    }

    int w, h;
    w = v->getViewportL()->calcPixelWidth();
    h = v->getViewportL()->calcPixelHeight();

    float rx, ry;
    v->getViewportL()->calcNormalizedCoordinates(rx, ry, x, y);


    //cam->getCam()->calcViewRay(ray,x,y,*v->getViewport());
    calcViewRay(cam, ray, rx,ry,w,h);

    editBeacon()->setDir(Vec3d(ray.getDirection()));
    editBeacon()->setFrom(Vec3d(ray.getPosition()));

    // TODO: This would be the better variant (compare VRMultitouch), but causes a compilation error where VRMultitouch does not.
//    auto p = Pose::create(Vec3d(ray.getPosition()), Vec3d(ray.getDirection()));
//    p->makeUpOrthogonal();
//    editBeacon()->setPose(p);

    int side = -1;
    if (rx > 0.95) side = 0;
    if (rx < -0.95) side = 1;
    if (ry > 0.95) side = 2;
    if (ry < -0.95) side = 3;
    if (rx > 0.95 && ry > 0.95) side = 4;
    if (rx < -0.95 && ry < -0.95) side = 5;
    if (rx > 0.95 && ry < -0.95) side = 6;
    if (rx < -0.95 && ry > 0.95) side = 7;

    if (side != onEdge) {
        sig_state = (side == -1) ? 5 : 4;
        sig_key = (side == -1) ? (1+v->getID())*10+onEdge : (1+v->getID())*10+side;
        if (side == -1) on_from_edge->triggerPtr<VRDevice>();
        else on_to_edge->triggerPtr<VRDevice>();
    }
    onEdge = side;
}

/**
* Is invoked whenever a mouse button is pressed or released.
* @param button: id of the button. Mostly left button = 0, middle button = 1, right button  = 2
* @param state: state of the event. 0 = button pressed, 1 = button released
* @param x: x coordinate of mouse pointer on screen
* @param y: y coordinate of mouse pointer on screen
*/
void VRMouse::mouse(int button, int state, int x, int y) {
    auto sv = view.lock();
    if (!sv) return;
    ViewportMTRecPtr v = sv->getViewportL();
    if (!v) return;
    if (!v->getParent()) return;

    float _x, _y;
    v->calcNormalizedCoordinates(_x, _y, x, y);
    change_slider(5, _x);
    change_slider(6, _y);

    updatePosition(x,y);
    if (state) change_button(button,false);
    else change_button(button,true);
}

void VRMouse::motion(int x, int y) {
    auto sv = view.lock();
    if (!sv) return;
    ViewportMTRecPtr v = sv->getViewportL();
    if (!v) return;
    if (!v->getParent()) return;

    float _x, _y;
    v->calcNormalizedCoordinates(_x, _y, x, y);
    change_slider(5,_x);
    change_slider(6,_y);
    updatePosition(x,y);
}

void VRMouse::setCamera(VRCameraPtr cam) { this->cam = cam; }
void VRMouse::setViewport(VRViewPtr view) { this->view = view; }

Line VRMouse::getRay() { return ray; }

void VRMouse::save(XMLElementPtr e) {
    VRDevice::save(e);
}

void VRMouse::load(XMLElementPtr e) {
    VRDevice::load(e);
}

