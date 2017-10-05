#ifndef VRRAIN_H_INCLUDED
#define VRRAIN_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/VRTransform.h"
#include <OpenSG/OSGColor.h>


//#include "addons/Bullet/Particles/VRParticles.h" //do i need this?
//#include "VRSky.h"


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
        struct Position {
            float latitude = 0;
            float longitude = 0;
        };
        struct Date {
            double second = 0;
            int hour = 12;
            int day = 0;
            int year = 2000;

            void propagate(double seconds); // positive or negative
            double getDay();

        };
        // -------------------------------------------------------------------------------------
        VRUpdateCbPtr updatePtr;
        VRMaterialPtr mat;
        string vScript;
        string fScript;

        // time and location
        double lastTime = 0;
        Date date;
        float speed = 1; // how quickly time passes
        Position observerPosition;

        // clouds
        uint textureSize;
        Vec2d cloudVel;
        Vec2f cloudOffset;
        float cloudDensity;
        float rainOffset;
        float cloudScale;
        float cloudHeight;
        Color4f cloudColor;

        // luminance
        float turbidity;
        Vec3f xyY_z;
        Vec3f coeffsA;
        Vec3f coeffsB;
        Vec3f coeffsC;
        Vec3f coeffsD;
        Vec3f coeffsE;

        // sun
        Vec3f sunPos;
        float theta_s;

        void calculateZenithColor();
        void updateClouds(float dt);
        void sunFromTime();
        void setCloudVel(float x, float z);
        void update();
        // -------------------------------------------------------------------------------------

        double densityRainStart = 0.1;      //density of clouds at start of transition
        double speedRainStartX = 0.002;
        double speedRainStartY = 0.001;
        double colorRainStart = 1;
        double lightRainStart = 1;

        double densityRain;
        double speedRainX;
        double speedRainY;
        double colorRain;
        double lightRain;

        //VRSky rainSky = VRSKy(); need sky from scene
        //VRLight need Light from scene

        double durationTransition = 10;
        double scaleRain = 10;

        void setRainScale();
        void setupRain();
        void clearRain();

        //void updateRain(float dt);
        //void update();

    public:
        VRRain();
        ~VRRain();

        // -------------------------------------------------------------------------------------
        static VRRainPtr create();
        VRRainPtr ptr();

        void setTime(double second, int hour, int day, int year = 2000); // 0-3600, 0-24, 0-356, x
        void setSpeed(float speed = 1);
        void setClouds(float density, float scale, float height, Vec2d speed, Color4f color);
        void setLuminance(float turbidity);
        void setPosition(float latitude, float longitude);
        void reloadShader();
        // -------------------------------------------------------------------------------------

        //VRRainPtr ptr();
        //static VRRainPtr create(string name = "rain");
        //void reloadShader();

        void updateRain(float dt);

        void setRain( double durationTransition, double scaleRain );
        Vec2d getRain();

        void startRain();
        void stopRain();

        void overrideParameters( double densityRain, double speedRain, double colorRain, double lightRain );
};

OSG_END_NAMESPACE;

#endif // VRRAIN_H_INCLUDED
