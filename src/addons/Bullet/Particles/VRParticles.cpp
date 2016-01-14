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


VRParticles::VRParticles() : VRParticles(true) {}

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
            colors->setValue(Vec4f(0,0,1,1),i);
        }
    }
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

int VRParticles::spawnCuboid(Vec3f base, Vec3f size, float distance) {
    if (distance == 0.0) distance = this->particles[0]->radius;

    // distance = abs(distance);
    // float x = abs(size.x());
    // float y = abs(size.y());
    // float z = abs(size.z());
    float x = size.x();
    float y = size.y();
    float z = size.z();

    //distance *= 1.1;
    int numX = x / distance;
    int numY = y / distance;
    int numZ = z / distance;
    int spawned = 0;
    bool done = false;
    int i,j,k;
    int posX, posY, posZ;
    btVector3 pos;

    {
        //BLock lock(mtx()); // NOTE causes buggy physics?!?
        for (i = 0; i < numY && !done; i++) {
            posY = i * distance;

            for (j = 0; j < numZ && !done; j++) {
                posZ = j * distance;

                for (k = 0; k < numX && !done; k++) {
                    posX = k * distance;

                    if (spawned >= this->N) {
                        done = true;
                    } else {
                        pos.setX(posX);
                        pos.setY(posY);
                        pos.setZ(posZ);
                        pos += toBtVector3(base);
                        particles[spawned]->spawnAt(pos, this->world, this->collideWithSelf);
                        spawned++;
                    }
                }
            }
        }
    }
    printf("Spawned %i particles!\n", spawned);
    setFunctions(0, spawned);
    return spawned;
}

void VRParticles::setEmitter(Vec3f baseV, Vec3f dirV, int from, int to, int interval, bool loop, float offsetFactor) {
    btVector3 base = this->toBtVector3(baseV);
    btVector3 dir = this->toBtVector3(dirV);
    {
        BLock lock(mtx());
        VRScenePtr scene = VRSceneManager::getCurrent();
        scene->dropUpdateFkt(emit_fkt);

        this->emit_base = base;
        this->emit_dir = dir;
        this->emit_from = from;
        this->emit_to = to;
        this->emit_interval = interval;
        this->emit_counter = 0;
        this->emit_i = from;
        this->emit_loop = false;


        for(int i=from; i < to; i++) {
            particles[i]->setup(emit_base + dir.normalized() * offsetFactor, false);
        }

        emit_fkt = VRFunction<int>::create("emitter", boost::bind(&VRParticles::emitterLoop, this));
        scene->addUpdateFkt(emit_fkt);
    }
        setFunctions(from, to);
        printf("VRParticles::setEmitter(...from=%i, to=%i, interval=%i)", from, to, interval);
}

void VRParticles::disableEmitter() {
    {
        BLock lock(mtx());
        VRScenePtr scene = VRSceneManager::getCurrent();
        scene->dropUpdateFkt(emit_fkt);
    }
}

/**
 * Emits one particle at a time
 */
void VRParticles::emitterLoop() {
    if (emit_counter == 1) {
        Particle* p = particles[emit_i];
        p->spawnAt(emit_base, this->world, this->collideWithSelf);
        printf("Emitter: Particle emitted\n");
        p->body->applyCentralForce(emit_dir);
        emit_i++;
        emit_counter++;
        if (emit_i == emit_to) {
            if (emit_loop) {
                //emit_i = emit_from;
                // TODO enable looped emitter by extending p->spawnAt()
                this->disableEmitter();
            } else {
                this->disableEmitter();
            }
        }
    } else if (emit_counter >= emit_interval-1) {
        emit_counter = 0;
    } else {
        emit_counter++;
    }
}

void VRParticles::setFunctions(int from, int to) {
    {
        BLock lock(mtx());
        this->from = from;
        this->to = to;
        VRScenePtr scene = VRSceneManager::getCurrent();
        scene->dropUpdateFkt(fkt);
        fkt = VRFunction<int>::create("particles_update", boost::bind(&VRParticles::update, this,from,to));
        scene->addUpdateFkt(fkt);
    }
    printf("VRParticles::setFunctions(from=%i, to=%i)", from, to);
}

void VRParticles::disableFunctions() {
    {
        BLock lock(mtx());
        VRScenePtr scene = VRSceneManager::getCurrent();
        scene->dropUpdateFkt(fkt);
    }
}
