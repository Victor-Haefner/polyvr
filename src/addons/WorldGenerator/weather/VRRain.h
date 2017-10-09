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

        // -------------------------------------------------------------------------------------
        VRUpdateCbPtr updatePtr;
        VRMaterialPtr mat;
        string vScript;
        string fScript;
        float density = 0;
        float offset = 0;

        // time and location
        double lastTime = 0;

        // clouds
        uint textureSize;

        void update();
        // -------------------------------------------------------------------------------------

        double densityStart = 0.1;      //density of clouds at start of transition
        double speedStartX = 0.002;
        double speedStartY = 0.001;
        double colorStart = 1;
        double lightStart = 1;

        double speedX;
        double speedY;
        double color;
        double light;

        //VRSky rainSky = VRSKy(); need sky from scene
        //VRLight need Light from scene

        double durationTransition = 10;
        double scale = 0;
        double scaleRN;

        //void updateRain(float dt);
        //void update();

    public:
        VRRain();
        ~VRRain();

        // -------------------------------------------------------------------------------------
        static VRRainPtr create();
        VRRainPtr ptr();


        void reloadShader();
        // -------------------------------------------------------------------------------------

        //VRRainPtr ptr();
        //static VRRainPtr create(string name = "rain");
        //void reloadShader();

        void updateRain(float dt);

        void setScale( double scale );
        Vec2d get();

        void start();
        void stop();

        void overrideParameters( double density, double speed, double color, double light );
};

OSG_END_NAMESPACE;

#endif // VRRAIN_H_INCLUDED
