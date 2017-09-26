#ifndef VRRAIN_H_INCLUDED
#define VRRAIN_H_INCLUDED

#include "addons/Bullet/Particles/VRParticles.h" //do i need this?

#include "core/objects/VRTransform.h"
//#include "addons/Bullet/Particles/VREmitter.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

//class VRParticles;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRain : public VRTransform { //: public VRParticles {
    private:
        int lifetime;
        double mass;

        double densityRainStart = 0.1;
        double speedRainStart = 0.001;
        double colorRainStart = 1;
        double lightRainStart = 1;

        double densityRain;
        double speedRain;
        double colorRain;
        double lightRain;

        double durationTransition = 10;
        double scaleRain = 10;



        void setRainScale();
        void setupRain();

    public:
        VRRain();
        ~VRRain();

        VRRainPtr ptr();
        static VRRainPtr create(string name = "rain");
        //static shared_ptr<VRRain> create();

        void setRain( double durationTransition, double scaleRain );
        Vec2d getRain();

        void startRain();
        void stopRain();

        void overrideParameters( double densityRain, double speedRain, double colorRain, double lightRain );
};

OSG_END_NAMESPACE;

#endif // VRRAIN_H_INCLUDED
