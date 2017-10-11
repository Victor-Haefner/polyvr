#include "VRMultiTouch.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRGtkWindow.h"
#include "core/objects/VRCamera.h"
#include "core/objects/OSGCamera.h"
#include "VRSignal.h"
#include <GL/glut.h>
#include <OpenSG/OSGPerspectiveCamera.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRMultiTouch::VRMultiTouch() : VRDevice("multitouch") {}
VRMultiTouch::~VRMultiTouch() {}

VRMultiTouchPtr VRMultiTouch::create() {
    auto m = VRMultiTouchPtr(new VRMultiTouch());
    m->initIntersect(m);
    m->clearSignals();
    return m;
}

VRMultiTouchPtr VRMultiTouch::ptr() { return static_pointer_cast<VRMultiTouch>( shared_from_this() ); }

void VRMultiTouch::setCursor(string c) {
    auto s = VRSetup::getCurrent();
    for (auto w : s->getWindows()) {
        if (!w.second->hasType(2)) continue; // not a gtk window
        auto win = dynamic_pointer_cast<VRGtkWindow>(w.second);
        win->setCursor(c);
    }
}

void VRMultiTouch::clearSignals() {
    VRDevice::clearSignals();
    addSlider(5);
    addSlider(6);

    addSignal( 0, 0)->add( getDrop() );
    addSignal( 0, 1)->add( addDrag( getBeacon() ) );

    if (on_to_edge) on_to_edge->clear();
    if (on_from_edge) on_from_edge->clear();
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

VRSignalPtr VRMultiTouch::getToEdgeSignal() { return on_to_edge; }
VRSignalPtr VRMultiTouch::getFromEdgeSignal() { return on_from_edge; }

//3d object to emulate a hand in VRSpace
void VRMultiTouch::updatePosition(int x, int y) {
    auto cam = this->cam.lock();
    if (!cam) return;
    auto v = view.lock();
    if (!v) return;

    float rx, ry;
    int w, h;
    w = v->getViewport()->calcPixelWidth();
    h = v->getViewport()->calcPixelHeight();
    v->getViewport()->calcNormalizedCoordinates(rx, ry, x, y);

    //cam->getCam()->calcViewRay(ray,x,y,*v->getViewport());
    calcViewRay(cam, ray, rx,ry,w,h);
    editBeacon()->setDir(Vec3d(ray.getDirection()));

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
