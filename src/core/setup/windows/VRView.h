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
#include "core/scene/VRRenderManager.h"

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRLight;
class VRDefShading;
class VRSSAO;
class VRHMDDistortion;
class VRCamera;

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

        Vec4f position = Vec4f(0,0,1,1);
        Vec2i window_size = Vec2i(300,300);

        float eyeSeparation = 0.06;
        bool eyeinverted = false;

        //stereo decorator
        ProjectionCameraDecoratorRecPtr PCDecoratorLeft;
        ProjectionCameraDecoratorRecPtr PCDecoratorRight;

        WindowRecPtr window;
        VRRenderStudioPtr renderingL;
        VRRenderStudioPtr renderingR;

        //ShearedStereoCameraDecoratorPtr SSCDecoratorLeft;
        //ShearedStereoCameraDecoratorPtr SSCDecoratorRight;

        Vec3f proj_user = Vec3f(0,0,0);
        Vec3f proj_center = Vec3f(0,0,-1);
        Vec3f proj_up = Vec3f(0,1,0);
        Vec3f proj_normal = Vec3f(0,0,1);
        Vec2f proj_size = Vec2f(2,1);
        Vec2f proj_shear = Vec2f(0,0);
        Vec2f proj_warp = Vec2f(0,0);

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
        SimpleStatisticsForegroundRecPtr stats = 0;
        GrabForegroundRecPtr grabfg = 0;
        ImageForegroundRecPtr calib_fg;

        map<string, VRGeometryPtr> renderLayer;
        VRMaterialPtr setupRenderLayer(string name, VRObjectPtr parent);
        void initCalib(VRMaterialPtr mat);

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
        void resize(Vec2i s);

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
        void setProjectionUser(Vec3f v);
        Vec3f getProjectionUser();
        void setProjectionSize(Vec2f v);
        Vec2f getProjectionSize();
        void setProjectionShear(Vec2f v);
        Vec2f getProjectionShear();
        void setProjectionWarp(Vec2f v);
        Vec2f getProjectionWarp();

        void showStats(bool b);
        void showViewGeo(bool b);

        Vec4f getPosition();
        void setPosition(Vec4f pos);

        ViewportRecPtr getViewport();
        VRRenderStudioPtr getRenderingL();
        VRRenderStudioPtr getRenderingR();

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
