#ifndef VRPARTICLES_H_INCLUDED
#define VRPARTICLES_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGGeoProperties.h>
#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/VRFunctionFwd.h"
#include "VRParticle.h"

class btDiscreteDynamicsWorld;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRParticles : public VRGeometry {
    // FIXME: Particles do not collide in ~50% of all polyvr sessions. Restart polyvr until it works.

    public:
        VRParticles();
        VRParticles(bool spawnParticles);
        ~VRParticles();

        static shared_ptr<VRParticles> create();

        void setAmount(int amount);
        void setRadius(float newRadius, float variation=0.0);
        virtual void setMass(float newMass, float variation=0.0);
        void setMassByRadius(float massFor1mRadius=1000.0);
        void setMassForOneLiter(float massPerLiter=0.1);
        void setAge(int newAge, int variation=0);
        void setLifetime(int newLifetime, int variation=0);

        template<class P> void resetParticles();
        int spawnCuboid(Vec3f base, Vec3f size, float distance = 0.0);
        virtual void update(int b = 0, int e = -1);

    protected:
        int N = 200;
        int from, to;
        bool collideWithSelf = true;
        vector<Particle*> particles;

        VRUpdatePtr fkt;
        VRMaterialPtr mat;
        GeoPnt3fPropertyRecPtr pos;
        GeoVec3fPropertyRecPtr normals;
        GeoVec4fPropertyRecPtr colors;
        btDiscreteDynamicsWorld* world = 0;

        boost::recursive_mutex& mtx();
        inline Vec3f toVec3f(btVector3 v) { return Vec3f(v[0], v[1], v[2]); };
        inline btVector3 toBtVector3(Vec3f v) { return btVector3(v[0], v[1], v[2]); };

        virtual void setFunctions(int from, int to);
};


OSG_END_NAMESPACE;

#endif // VRPARTICLES_H_INCLUDED
