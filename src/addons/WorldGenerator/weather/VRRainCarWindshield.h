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

        float wiperSpeed = 0.5;

        uint textureSize;

        void update();

        Vec3f convertV3dToV3f(Vec3d in);

        float scale = 10;

        float tnow = 0;
        double tlast = 0;
        float tdelta = 0;

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
