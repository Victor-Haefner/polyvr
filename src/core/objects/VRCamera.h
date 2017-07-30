#ifndef VRCAMERA_H_INCLUDED
#define VRCAMERA_H_INCLUDED

#include "VRTransform.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRCamera : public VRTransform {
    private:
        OSGCameraPtr cam;
        OSGObjectPtr camGeo;

        float parallaxD = 1;
        float nearClipPlaneCoeff = 0.1;
        float farClipPlaneCoeff = 512;
        float aspect = 1;
        float fov = 0;

        bool doAcceptRoot = true;

    public:
        VRCamera(string name = "");
        ~VRCamera();

        static VRCameraPtr create(string name = "None", bool reg = true);
        VRCameraPtr ptr();

        void setup();

        int camID = -1;
        void activate();

        OSGCameraPtr getCam();

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

        void focus(Vec3d p);
        void focus(VRObjectPtr t);
};

OSG_END_NAMESPACE;

#endif // VRCAMERA_H_INCLUDED
