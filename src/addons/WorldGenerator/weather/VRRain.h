#ifndef VRRAIN_H_INCLUDED
#define VRRAIN_H_INCLUDED

#include "VRParticle.h" //do i need this?
#include "VREmitter.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRain : public VRParticles {
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

        double durationTransition;
        double scaleRain;

        void setScaleRain();

    public:
        VRRain();
        ~VRRain();

        void setRain( double durationTransition, double scaleRain );
        Vec2d getRain();

        void startRain();
        void stopRain();

        void overrideParameters( double densityRain, double speedRain, double colorRain, double lightRain );
};

OSG_END_NAMESPACE;

#endif // VRRAIN_H_INCLUDED
