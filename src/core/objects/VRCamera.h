#ifndef VRCAMERA_H_INCLUDED
#define VRCAMERA_H_INCLUDED

#include <OpenSG/OSGPerspectiveCamera.h>
#include "VRTransform.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRCamera : public VRTransform {
    private:
        PerspectiveCameraRecPtr cam;
        NodeRecPtr camGeo;

        float parallaxD = 1;
        float nearClipPlaneCoeff = 0.1;
        float farClipPlaneCoeff = 512;
        float aspect = 1;
        float fov = 0;

        bool doAcceptRoot = true;

    protected:

        void loadContent(xmlpp::Element* e);

    public:
        VRCamera(string name = "");
        ~VRCamera();

        static VRCameraPtr create(string name = "None");
        VRCameraPtr ptr();

        void setup();

        int camID = -1;
        void activate();

        PerspectiveCameraRecPtr getCam();

        void setAcceptRoot(bool b);
        bool getAcceptRoot();

        float getAspect();
        float getFov();
        float getNear();
        float getFar();
        void setAspect(float a);
        void setFov(float f);
        void setNear(float f);
        void setFar(float f);
        void setProjection(string p);
        string getProjection();

        void showCamGeo(bool b);

        static list<VRCameraWeakPtr>& getAll();
        static vector<string> getProjectionTypes();

        void focus(Vec3f p);
        void focus(VRTransformPtr t);
};

OSG_END_NAMESPACE;

#endif // VRCAMERA_H_INCLUDED
