#ifndef VRRAINCARWINDSCHIELD_H_INCLUDED
#define VRRAINCARWINDSCHIELD_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/VRTransform.h"
#include <OpenSG/OSGColor.h>

#include "core/objects/VRCamera.h"
#include "core/objects/geometry/VRGeometry.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRainCarWindshield : public VRGeometry {
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
        //string vScriptTex;
        //string fScriptTex;
        float offset = 0;
        float camH = 40;
        float rainDensity = 0;

        bool isRaining = false;

        uint textureSize;

        void update();
        void startRainCallback(float t);
        void stopRainCallback(float t);
        void updateScale(float scaleNow);

        //inital cloud parameters
        float densityStart = 0.1;
        float speedStartX = 0.002;
        float speedStartY = 0.001;
        float colorStart = 1;
        float lightStart = 1;

        float scale;
        float density;
        float speedX;
        float speedY;
        float color;
        float light;

        double tnow = 0;
        double tlast = 0;
        double tdelta = 0;

        Vec3f carOrigin;
        Vec3f carDir;
        Vec3f posOffset;

    public:
        VRRainCarWindshield();
        ~VRRainCarWindshield();

        static VRRainCarWindshieldPtr create();
        VRRainCarWindshieldPtr ptr();

        float get();

        void doTestFunction();

};

OSG_END_NAMESPACE;

#endif // VRRAINCARWINDSCHIELD_H_INCLUDED
