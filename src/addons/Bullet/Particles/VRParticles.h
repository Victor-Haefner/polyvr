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

        static const int startValue = 400;
        int N = startValue;

        void setRadius(float newRadius, float variation=0.0);
        virtual void setMass(float newMass, float variation=0.0);
        void setMassByRadius(float massFor1mRadius=1000.0);
        void setMassForOneLiter(float massPerLiter=0.1);
        void setAge(int newAge, int variation=0);
        void setLifetime(int newLifetime, int variation=0);

        template<class P> void resetParticles(int amount=startValue);
        virtual void update(int b = 0, int e = -1);
        void emitterLoop();
        int spawnCuboid(Vec3f base, Vec3f size, float distance = 0.0);
        virtual void setEmitter(Vec3f base, Vec3f dir, int from, int to, int interval, bool loop=false, float offsetFactor=0);
        void disableEmitter();

    protected:
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
        virtual void disableFunctions();

        /* Emitter variables */
        VRUpdatePtr emit_fkt;
        btVector3 emit_base;
        btVector3 emit_dir;
        int emit_from = 0;
        int emit_to = N;
        int emit_interval = 60;
        int emit_counter = 0;
        int emit_i = 0;
        bool emit_loop = false;


};


OSG_END_NAMESPACE;

#endif // VRPARTICLES_H_INCLUDED
