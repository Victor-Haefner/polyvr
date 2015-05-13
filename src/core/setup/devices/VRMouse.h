#ifndef VRMOUSE_H_INCLUDED
#define VRMOUSE_H_INCLUDED

#include "core/setup/windows/VRView.h"
#include "VRDevice.h"
#include <OpenSG/OSGLine.h>
#include <OpenSG/OSGPerspectiveCamera.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRMouse : public VRDevice {
    private:

        VRCamera* cam;
        VRView* view;
        Line ray;

        VRSignal* on_to_edge = 0;
        VRSignal* on_from_edge = 0;
        int onEdge = -1;

        void multFull(Matrix _matrix, const Pnt3f &pntIn, Pnt3f  &pntOut);

        bool calcViewRay(PerspectiveCameraRecPtr pcam, Line &line, float x, float y, int W, int H);

    public:
        VRMouse();

        void clearSignals();

        //3d object to emulate a hand in VRSpace
        void updatePosition(int x, int y);

        void mouse(int button, int state, int x, int y);
        void motion(int x, int y);

        void setCamera(VRCamera* _cam);
        void setViewport(VRView* _view);

        Line getRay();
        VRSignal* getToEdgeSignal();
        VRSignal* getFromEdgeSignal();

        void save(xmlpp::Element* e);
        void load(xmlpp::Element* e);
};

OSG_END_NAMESPACE;

#endif // VRMOUSE_H_INCLUDED
