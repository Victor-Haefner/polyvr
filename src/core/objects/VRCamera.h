#ifndef VRCAMERA_H_INCLUDED
#define VRCAMERA_H_INCLUDED

#include "VRTransform.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRCamera : public VRTransform {
    public:
        enum TYPE{
            PERSPECTIVE = 0,
            ORTHOGRAPHIC
        };

    private:
        OSGCameraPtr cam;
        OSGObjectPtr camGeo;
        VRObjectPtr vrSetup;

        bool registred = true;
        int camType = 0;
        float parallaxD = 1;
        float nearClipPlaneCoeff = 0.1;
        float farClipPlaneCoeff = 512;
        float aspect = 1;
        float fov = 0;
        float orthoSize = 100;

        bool doAcceptRoot = true;

        void updateOrthSize();

    public:
        VRCamera(string name = "");
        ~VRCamera();

        static VRCameraPtr create(string name = "None", bool reg = false);
        VRCameraPtr ptr();

        VRObjectPtr copy(vector<VRObjectPtr> children);

        void setType(int type);
        int getType();
        void setup(bool reg = true, VRStorageContextPtr context = 0);

        int camID = -1;
        void activate();

        OSGCameraPtr getCam();
        void setCam(OSGCameraPtr c);

        void setAcceptRoot(bool b);
        bool getAcceptRoot();
        VRObjectPtr getSetupNode();

        float getAspect();
        float getFov();
        float getNear();
        float getFar();
        float getOrthoSize();
        void setAspect(float a);
        void setFov(float f);
        void setNear(float f);
        void setFar(float f);
        void setOrthoSize(float f);
        void setProjection(string p);
        string getProjection();

        Matrix getProjectionMatrix(int w, int h);

        void showCamGeo(bool b);

        static list<VRCameraWeakPtr>& getAll();
        static vector<string> getProjectionTypes();

        void focusPoint(Vec3d p);
        void focusObject(VRObjectPtr t);

        void setAt(Vec3d at);
        void setFrom(Vec3d pos);
        void setMatrix(Matrix4d m);
};

OSG_END_NAMESPACE;

#endif // VRCAMERA_H_INCLUDED
