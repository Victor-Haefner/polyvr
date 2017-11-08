#ifndef VRRAIN_H_INCLUDED
#define VRRAIN_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/VRTransform.h"
#include <OpenSG/OSGColor.h>


//#include "addons/Bullet/Particles/VRParticles.h" //do i need this?
//#include "VRSky.h"

#include "core/objects/VRCamera.h"
#include "core/objects/geometry/VRGeometry.h"
//#include "addons/Bullet/Particles/VREmitter.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

//class VRParticles;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRain : public VRGeometry { //: public VRParticles {
    private:
        //int lifetime;
        //double mass;

        // -------------------------------------------------------------------------------------
        VRUpdateCbPtr updatePtr;
        VRAnimCbPtr rainAnimationCb;
        VRMaterialPtr mat;
        VRMaterialPtr matTex;
        VRMaterialPtr renderMat;
        VRLightPtr lightMain;
        //VRTextureRendererPtr tr;

        VRCameraPtr camTex;
        VRTextureRendererPtr texRenderer;
        VRGeometryPtr cube;
        //VRSkyPtr sky;
        string vScript;
        string fScript;
        string vScriptTex;
        string fScriptTex;
        float offset = 0;
        float camH = 40;
        float rainDensity = 0;
        //
        // time and location
        float lastTime = 0;
        bool isRaining = false;

        // clouds
        uint textureSize;

        void update();
        void startRainCallback(float t);
        void stopRainCallback(float t);
        void updateScale(float scaleNow);
        // -------------------------------------------------------------------------------------
        float densityStart = 0.1;      //density of clouds at start of transition
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

        //VRSky rainSky = VRSKy(); need sky from scene
        //VRLight need Light from scene

        float durationTransition = 10;
        float scale = 0;
        float scaleRN = 0;

        //void updateRain(float dt);

    public:
        VRRain();
        ~VRRain();

        // -------------------------------------------------------------------------------------
        static VRRainPtr create();
        VRRainPtr ptr();


        void reloadShader();
        // -------------------------------------------------------------------------------------

        void setScale( float scale );
        Vec2d get();

        void start();
        void stop();

        void overrideParameters( float density, float speed, float color, float light );
};

OSG_END_NAMESPACE;

#endif // VRRAIN_H_INCLUDED
