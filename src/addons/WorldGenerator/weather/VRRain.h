#ifndef VRRAIN_H_INCLUDED
#define VRRAIN_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/VRTransform.h"
#include <OpenSG/OSGColor.h>

#include "core/objects/VRCamera.h"
#include "core/objects/geometry/VRGeometry.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRain : public VRGeometry {
    private:
        bool debugRain = false;
        VRUpdateCbPtr updatePtr;
        VRAnimCbPtr rainAnimationCb;
        VRMaterialPtr mat;
        VRMaterialPtr renderMat;
        VRLightPtr lightMain;

        VRCameraPtr camTex;
        VRCameraPtr oldCamTex;
        VRTextureRendererPtr texRenderer;
        VRGeometryPtr cube;

        string vScript;
        string fScript;
        string dfScript;
        string vScriptTex;
        string fScriptTex;
        string dfScriptTex;
        float offset = 0;
        float camH = 40;
        float rainDensity = 0;
        Vec3f dropColor;
        float dropWidth;
        float dropLength;

        bool isRaining = false;
        bool depthTexer = false;

        uint textureSize;

        void update();
        Vec3f convertV3dToV3f(Vec3d in);
        void startRainCallback(float t);
        void stopRainCallback(float t);
        void updateScale(float scaleNow);
        void setupShaderLocations();

        //inital cloud parameters
        float densityStart = 0.1;
        float speedStartX = 0.002;
        float speedStartY = 0.001;
        float colorStart = 1;
        float lightStart = 1;

        float density;
        float speedX;
        float speedY;
        float color;
        float light;

        float tnow = 0;

        float durationTransition = 10;  //t in seconds
        float scale = 0;
        float scaleRN = 0;

    public:
        VRRain();
        ~VRRain();

        static VRRainPtr create();
        VRRainPtr ptr();

        float get();
        VRTextureRendererPtr getRenderer();
        VRMaterialPtr getTexMat();

        void setDropColor(Vec3d clIn);
        void setDropSize(float dropWidth, float dropLength);

        void setScale( bool liveChange , float scale );
        void doTestFunction();

        void start();
        void stop();
        bool getIsRaining();

        void overrideParameters( float durationTransition, float rainDensity, float density, float speedX, float speedY, float color, float light );
        void reloadShader();
};

OSG_END_NAMESPACE;

#endif // VRRAIN_H_INCLUDED
