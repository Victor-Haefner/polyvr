#ifndef VRPARTICLES_H_INCLUDED
#define VRPARTICLES_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGGeoProperties.h>
#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/VRFunction.h"

class btDiscreteDynamicsWorld;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRGeometry;
struct Particle;

class VRParticles : public VRGeometry {
    // FIXME: PolyVR crashes when Particles are not spawned or emitted and script is executed the second time.
    // FIXME: Particles do not collide in ~50% of all polyvr sessions. Restart polyvr until it works.

    protected:
        int N = 1000;
        vector<Particle*> particles;

        VRFunction<int>* fkt = 0;
        VRMaterial* mat = 0;
        GeoPnt3fPropertyRecPtr pos;
        btDiscreteDynamicsWorld* world = 0;

        float getMaxRadius();

    public:
        enum ArgType { NOTHING, SIZE, LITER };

        VRParticles(): VRParticles(400){}
        VRParticles(int particleAmount);
        ~VRParticles();

        void setRadius(float newRadius, float variation=0.0);
        void setMass(float newMass, float variation=0.0);
        void setMassByRadius(float massFor1mRadius=1000.0);
        void setMassForOneLiter(float massPerLiter=0.1);
        void setAge(int newAge, int variation=0);
        void setLifetime(int newLifetime, int variation=0);

        template<class P> void initParticles();
        int spawnCuboid(Vec3f v, ArgType t=NOTHING, float a=1, float b=1, float c=1);
        void update(int b = 0, int e = -1);
};


OSG_END_NAMESPACE;

#endif // VRPARTICLES_H_INCLUDED
