#ifndef VRVIEW_H_INCLUDED
#define VRVIEW_H_INCLUDED

#include <OpenSG/OSGProjectionCameraDecorator.h>
#include <OpenSG/OSGSolidBackground.h>
#include <OpenSG/OSGStereoBufferViewport.h>
#include <OpenSG/OSGSimpleStatisticsForeground.h>
#include <OpenSG/OSGGrabForeground.h>
#include <OpenSG/OSGImageForeground.h>
#include "core/objects/VRObjectFwd.h"
#include "core/setup/VRSetupFwd.h"

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRView : public std::enable_shared_from_this<VRView> {
    private:
        int ID;
        string name;

        bool active_stereo = false;
        bool stereo = false;
        bool projection = false;
        bool doStats = false;

        NodeMTRecPtr viewGeo;
        VRMaterialPtr viewGeoMat;

        Vec4f position;

        float eyeSeparation;
        bool eyeinverted = false;

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
        VRObjectPtr view_root;
        VRTransformPtr real_root;
        VRTransformPtr user;
        VRTransformPtr dummy_user;
        string user_name;
        VRCameraPtr cam;

        BackgroundRecPtr background;
        SimpleStatisticsForegroundRecPtr stats;
        GrabForegroundRecPtr grabfg;
        ImageForegroundRecPtr calib_fg;

        void setMaterial();
        void setViewports();
        void setDecorators();
        void setCam();
        void setBG();
        void setUser();
        void setWindow();
        void setRoot();

    public:
        VRView(string name);
        ~VRView();

        static VRViewPtr create(string name);
        VRViewPtr ptr();

        int getID();
        void setID(int i);

        string getName();

        void setRoot(VRObjectPtr root, VRTransformPtr _real);
        void setUser(VRTransformPtr u);
        void setCamera(VRCameraPtr c);
        void setBackground(BackgroundRecPtr bg);
        void setWindow(WindowRecPtr win);
        void setStereo(bool b);
        void setStereoEyeSeparation(float v);
        void setProjection(bool b);

        VRTransformPtr getUser();
        VRCameraPtr getCamera();
        bool isStereo();
        float getEyeSeparation();
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
        void setActiveStereo(bool b);
        bool activeStereo();

        void update();
        void reset();

        void setFotoMode(bool b);
        VRTexturePtr grab();

        void save(xmlpp::Element* node);
        void load(xmlpp::Element* node);
};

OSG_END_NAMESPACE;

#endif // VRVIEW_H_INCLUDED
