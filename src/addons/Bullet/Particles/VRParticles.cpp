#include "VRParticles.h"
#include "VRParticlesT.h"
#include "VRParticle.h"
#include "VREmitter.h"
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

VRParticles::VRParticles(bool spawnParticles) : VRGeometry("particles"), ocparticles(0.1) {
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
            if (particles[i]->isActive) {
                auto p = particles[i]->body->getWorldTransform().getOrigin();
                pos->setValue(toVec3f(p),i);
                colors->setValue(Vec4f(0,0,1,1),i);
            }
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

int VRParticles::setEmitter(Vec3f baseV, Vec3f dirV, int from, int to, int interval, bool loop) {
    if (to > particles.size() || from > particles.size() || from > to) {
        printf("ERROR: Please check parameters \'from\' and \'to\'\n");
        printf("ERROR: No Emitter was created.");
        return -1;
    }
    btVector3 base = this->toBtVector3(baseV);
    btVector3 dir = this->toBtVector3(dirV);

    // create vector with relevant particles
    vector<Particle*> p;
    p.resize(to-from,0);
    for (int i=from; i < to; i++) {
        p[i-from] = particles[i];
    }

    // set up emitter and insert into emitter map
    Emitter* e = new Emitter(world, p, base, dir, interval, this->collideWithSelf);
    // e->setLoop(loop); // TODO implement loop
    this->emitters[e->id] = e; //store emitters

    setFunctions(from, to);
    e->setActive(true);
    printf("VRParticles::setEmitter(...from=%i, to=%i, interval=%i)\n", from, to, interval);
    return e->id;
}

void VRParticles::disableEmitter(int id) {
    this->emitters[id]->setActive(false);
}

void VRParticles::destroyEmitter(int id) {
    this->disableEmitter(id);
    this->emitters.erase(id);
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
}

void VRParticles::disableFunctions() {
    {
        BLock lock(mtx());
        VRScenePtr scene = VRSceneManager::getCurrent();
        scene->dropUpdateFkt(fkt);
    }
}
