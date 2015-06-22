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

        float parallaxD;
        float nearClipPlaneCoeff;
        float farClipPlaneCoeff;

        bool doAcceptRoot;

    protected:

        void saveContent(xmlpp::Element* e);
        void loadContent(xmlpp::Element* e);

    public:
        VRCamera(string name = "");
        ~VRCamera();

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

        static list<VRCamera*>& getAll();
        static vector<string> getProjectionTypes();
};

OSG_END_NAMESPACE;

#endif // VRCAMERA_H_INCLUDED
