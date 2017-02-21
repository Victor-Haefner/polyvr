#ifndef VRSKY_H_INCLUDED
#define VRSKY_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/scene/VRSceneManager.h"
#include "VRGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSky : public VRGeometry {
    protected:
        struct Date {
            double second = 0;
            int hour = 0;
            int day = 0;
            int year = 2000;

            void propagate(double seconds); // positive or negative
            double getDay();

        };

        VRUpdateCbPtr updatePtr;
        VRMaterialPtr mat;

        uint textureSize;
        Vec2f cloudVel;
        Vec2f cloudOffset;
        Vec3f sunFromTime();
        Vec3f sunPos;

        double lastTime;
        Date date;
        float speed; // how quickly time passes

        void update();
        void updateClouds(float dt);

    public:
        VRSky();
        ~VRSky();

        static VRSkyPtr create();
        VRSkyPtr ptr();
        void setTime(double second, int hour, int day, int year = 2000); // 0-3600, 0-24, 0-356, x
        void setCloudVel(float x, float z);
        void setSpeed(float speed = 1);
};

OSG_END_NAMESPACE;

#endif // VRSKY_H_INCLUDED
