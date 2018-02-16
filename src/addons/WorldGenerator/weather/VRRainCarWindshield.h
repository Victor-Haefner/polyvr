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
        VRMaterialPtr mat;

        VRTextureRendererPtr texRenderer;
        VRGeometryPtr cube;

        VRGeometryPtr geoWindshield;

        string vScript;
        string fScript;

        bool isRaining = false;
        bool isWiping = false;

        float wiperSpeed = 0;

        uint textureSize;

        void update();

        Vec3f convertV3dToV3f(Vec3d in);
        Vec2f convertV2dToV2f(Vec2d in);
        float vecLength(Vec3d in);
        Vec2f calcAccComp(Vec3d accelerationVec,Vec3d windshieldDir,Vec3d windshieldUp);

        float scale = 10;

        float tnow = 0;
        float tWiperstart = 0;
        float durationWiper = 10;
        double tlast = 0;
        float tdelta = 0;
        Vec3d lastWorldPosition = Vec3d(0,0,0);
        Vec3d lastVelocityVec = Vec3d(0,0,0);
        Vec2f oldMapOffset0 = Vec2f(0,0);
        Vec2f oldMapOffset1 = Vec2f(0,0);

        template<typename T> void setShaderParameter(string name, T t);

    public:
        VRRainCarWindshield();
        ~VRRainCarWindshield();

        static VRRainCarWindshieldPtr create();
        VRRainCarWindshieldPtr ptr();

        float get();
        void setWindshield(VRGeometryPtr geoWindshield);
        void setScale(bool liveChange, float scale);
        void setWipers(bool isWiping, float wiperSpeed);

        void doTestFunction();
        void start();
        void stop();

};

OSG_END_NAMESPACE;

#endif // VRRAINCARWINDSCHIELD_H_INCLUDED
