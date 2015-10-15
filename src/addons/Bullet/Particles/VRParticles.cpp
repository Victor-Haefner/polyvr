#include "VRParticles.h"
#include "VRParticlesT.h"
#include "VRParticle.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"

#include <cmath> /* cbrtf() */
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>

typedef boost::recursive_mutex::scoped_lock BLock;

using namespace std;
using namespace OSG;

boost::recursive_mutex& VRParticles::mtx() {
    auto scene = OSG::VRSceneManager::getCurrent();
    if (scene) return scene->physicsMutex();
    else {
        static boost::recursive_mutex m;
        return m;
    };
}


VRParticles::VRParticles() : VRParticles(true) {
    //resetParticles<Particle>();
}

VRParticles::VRParticles(bool spawnParticles) : VRGeometry("particles") {
    if (spawnParticles) resetParticles<Particle>();
}

VRParticles::~VRParticles() {
    VRScenePtr scene = VRSceneManager::getCurrent();
    if (scene) scene->dropUpdateFkt(fkt);

    {
        BLock lock(mtx());
        for (int i=0;i<N;i++) delete particles[i];
    }
}

shared_ptr<VRParticles> VRParticles::create() {
    return shared_ptr<VRParticles>( new VRParticles() );
}


void VRParticles::update(int b, int e) {
    if (e < 0) e = N;
    {
        BLock lock(mtx());
        for (int i=b; i < e; i++) {
            auto p = particles[i]->body->getWorldTransform().getOrigin();
            pos->setValue(toVec3f(p),i);
        }
    }
    setPositions(pos);
}

// NOTE @depricated (probably)
float VRParticles::getMaxRadius() {
    int i;
    float maximum = 0.0;
    {
        BLock lock(mtx());
        for (i=0; i<N; i++) {
            if (maximum < particles[i]->radius) maximum = particles[i]->radius;
        }
    }
    return maximum;
}

void VRParticles::setMass(float newMass, float variation) {
    int i;
    float result;
    {
        BLock lock(mtx());
        for (i=0; i<N; i++) {
            result = newMass;
            result += (2*variation) * ( ((float) rand()) / RAND_MAX );
            result -= variation;
            particles[i]->mass = result;
        }
    }
}

void VRParticles::setMassByRadius(float massFor1mRadius) {
    int i;
    float result;
    {
        BLock lock(mtx());
        for (i=0; i<N; i++) {
            result = this->particles[i]->radius;
            this->particles[i]->mass = massFor1mRadius * pow(result, 3);
        }
    }
}

void VRParticles::setMassForOneLiter(float massPerLiter) {
    this->setMassByRadius(10*massPerLiter);
}

void VRParticles::setRadius(float newRadius, float variation) {
    int i;
    float result;
    {
        BLock lock(mtx());
        for (i=0; i<N; i++) {
            result = newRadius;
            result += (2 * variation * float(rand()) / RAND_MAX );
            result -= variation;
            particles[i]->radius = result;
        }
    }
}

void VRParticles::setAge(int newAge, int variation) {
    int i, result;
    {
        BLock lock(mtx());
        for (i=0; i<N; i++) {
            result = newAge;
            result += (2*variation) * (rand() / RAND_MAX);
            result -= variation;
            particles[i]->age = result;
        }
    }
}

void VRParticles::setLifetime(int newLifetime, int variation) {
    int i, result;
    {
        BLock lock(mtx());
        for (i=0; i<N; i++) {
            result = newLifetime;
            result += (2*variation) * (rand() / RAND_MAX);
            result -= variation;
            particles[i]->lifetime = result;
        }
    }
}

int VRParticles::spawnCuboid(Vec3f base, ArgType type, float a, float b, float c) {
    VRScenePtr scene = VRSceneManager::getCurrent();
    // if (scene) world = scene->bltWorld();

    float radius;
    int required;
    radius = getMaxRadius();

    switch (type) {
        case SIZE:
            break;
        case NOTHING: // default case: spawn 10 Liter!
            a = 10;
            type = LITER;
            // go on with LITER (therefore no break;)
        case LITER:
            a = b = c = cbrtf(a);
            break;
    }
    required = (int)  ( 0.5 * (a/radius) * (b/radius) * (c/radius) );
    if (required > N) required = N;

    // now, randomly place particles in rectangle.
    // TODO place more intelligent (grid, not random) to avoid problems
    btVector3 v;
    float y,x,z; x=y=z=0;
    int i;
    {
        BLock lock(mtx());
        for (i=0; i<required; i++)
        {
            // FIXME: for some reason, base is still not the middle point between all particles.
            v.setZero();
            x = (a*float(rand())/RAND_MAX);
            x = x - (x/2);
            v.setX (x);

            y = (b*float(rand())/RAND_MAX);
            y = y - (y/2);
            v.setY (y);

            z = (c*float(rand())/RAND_MAX);
            z = z - (z/2);
            v.setZ (z);

            v += toBtVector3(base);
            particles[i]->spawnAt(v, this->world);
        }
        // activate update loop, only update spawned particles! (required)
        scene->dropUpdateFkt(fkt);
        fkt = VRFunction<int>::create("particles_update", boost::bind(&VRParticles::update, this,0,required));
        scene->addUpdateFkt(fkt);
    }
    return required;
}
