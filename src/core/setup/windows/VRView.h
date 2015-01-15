#ifndef VRVIEW_H_INCLUDED
#define VRVIEW_H_INCLUDED

#include <OpenSG/OSGProjectionCameraDecorator.h>
#include <OpenSG/OSGSolidBackground.h>
#include <OpenSG/OSGStereoBufferViewport.h>
#include <OpenSG/OSGSimpleStatisticsForeground.h>
#include <OpenSG/OSGGrabForeground.h>

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRMaterial;
class VRCamera;
class VRTransform;
class VRObject;

class VRView {
    private:
        int ID;
        string name;

        bool active_stereo;
        bool stereo;
        bool projection;
        bool doStats;

        NodeRecPtr viewGeo;
        VRMaterial* viewGeoMat;

        Vec4f position;

        float eyeSeparation;
        bool eyeinverted;

        //stereo decorator
        ProjectionCameraDecoratorRecPtr PCDecoratorLeft;
        ProjectionCameraDecoratorRecPtr PCDecoratorRight;

        WindowRecPtr window;

        //ShearedStereoCameraDecoratorPtr SSCDecoratorLeft;
        //ShearedStereoCameraDecoratorPtr SSCDecoratorRight;

        Vec3f proj_center;
        Vec3f proj_up;
        Vec3f proj_normal;
        Vec2f proj_size;

        Pnt3f screenLowerLeft;
        Pnt3f screenLowerRight;
        Pnt3f screenUpperRight;
        Pnt3f screenUpperLeft;

        //stereo viewports
        ViewportRecPtr lView;//used also for non stereo
        ViewportRecPtr rView;
        //active stereo
        StereoBufferViewportRecPtr lView_act;
        StereoBufferViewportRecPtr rView_act;

        //headtracking user
        VRObject* view_root;
        VRTransform* real_root;
        VRTransform* user;
        VRTransform* dummy_user;
        VRCamera* cam;

        BackgroundRecPtr background;
        SimpleStatisticsForegroundRecPtr stats;
        GrabForegroundRecPtr grabfg;

        void setMaterial();

        void setViewports();

        void setDecorators();

    public:
        //VRView(bool _active_stereo = false, bool _stereo = false, bool _projection = false, Pnt3f _screenLowerLeft = Pnt3f(0,0,0), Pnt3f _screenLowerRight = Pnt3f(0,0,0), Pnt3f _screenUpperRight = Pnt3f(0,0,0), Pnt3f _screenUpperLeft = Pnt3f(0,0,0), bool swapeyes = false);
        VRView(string n);
        ~VRView();

        int getID();
        void setID(int i);

        void setRoot(VRObject* root = 0, VRTransform* _real = 0);

        void setUser(VRTransform* u = 0);
        VRTransform* getUser();

        void setCamera(VRCamera* c = 0);
        VRCamera* getCamera();

        void setBackground(BackgroundRecPtr bg = 0);

        void setWindow(WindowRecPtr win = 0);

        void setStereo(bool b);
        bool isStereo();

        void setStereoEyeSeparation(float v);
        float getEyeSeparation();

        void setProjection(bool b);
        bool isProjection();

        void setProjectionUp(Vec3f v);
        Vec3f getProjectionUp();
        void setProjectionNormal(Vec3f v);
        Vec3f getProjectionNormal();
        void setProjectionCenter(Vec3f v);
        Vec3f getProjectionCenter();
        void setProjectionSize(Vec2f v);
        Vec2f getProjectionSize();

        void showStats(bool b);
        void showViewGeo(bool b);

        Vec4f getPosition();
        void setPosition(Vec4f pos);

        ViewportRecPtr getViewport();

        void swapEyes(bool b);
        bool eyesInverted();

        void update();
        void reset();

        void setFotoMode(bool b);
        void setCallibrationMode(bool b);
        ImageRecPtr grab();

        void save(xmlpp::Element* node);
        void load(xmlpp::Element* node);
};

OSG_END_NAMESPACE;

#endif // VRVIEW_H_INCLUDED
