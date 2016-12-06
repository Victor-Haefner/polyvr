#include "VRParticles.h"
#include "VRParticlesT.h"
#include "VRParticle.h"
#include "VREmitter.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/scene/VRScene.h"

#include <cmath> /* cbrtf() */
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>

#include <OpenSG/OSGGeometry.h>

typedef boost::recursive_mutex::scoped_lock BLock;

using namespace std;
using namespace OSG;

boost::recursive_mutex& VRParticles::mtx() {
    auto scene = OSG::VRScene::getCurrent();
    if (scene) return scene->physicsMutex();
    else {
        static boost::recursive_mutex m;
        return m;
    };
}

VRParticles::VRParticles(bool spawnParticles) : VRGeometry("particles"), ocparticles(0.1) {
    if (spawnParticles) resetParticles<Particle>();
    allowCulling(false);
    getMesh()->geo->setDlistCache(false);
}

VRParticles::~VRParticles() {
    disableFunctions();
    BLock lock(mtx());
    for (int i=0;i<N;i++) delete particles[i];
}

shared_ptr<VRParticles> VRParticles::create() { return shared_ptr<VRParticles>( new VRParticles() ); }

void VRParticles::updateParticles(int b, int e) {
    if (e < 0) e = N;

    BLock lock(mtx());
    for (int i=b; i < e; i++) {
        if (particles[i]->isActive) {
            auto p = particles[i]->body->getWorldTransform().getOrigin();
            pos->setValue(toVec3f(p),i);
            colors->setValue(Vec4f(0,0,1,1),i);
        }
    }
}

void VRParticles::setMass(float newMass, float variation) {
    int i;
    float result;

    BLock lock(mtx());
    for (i=0; i<N; i++) {
        result = newMass;
        result += (2*variation) * ( ((float) rand()) / RAND_MAX );
        result -= variation;
        particles[i]->mass = result;
    }
}

void VRParticles::setMassByRadius(float massFor1mRadius) {
    int i;
    float result;

    BLock lock(mtx());
    for (i=0; i<N; i++) {
        result = this->particles[i]->radius;
        this->particles[i]->mass = massFor1mRadius * pow(result, 3);
    }
}

void VRParticles::setMassForOneLiter(float massPerLiter) { this->setMassByRadius(10*massPerLiter); }

void VRParticles::setRadius(float newRadius, float variation) {
    int i;
    float result;

    BLock lock(mtx());
    for (i=0; i<N; i++) {
        result = newRadius;
        result += (2 * variation * float(rand()) / RAND_MAX );
        result -= variation;
        particles[i]->radius = result;
    }
}

void VRParticles::setAge(int newAge, int variation) {
    int i, result;

    BLock lock(mtx());
    for (i=0; i<N; i++) {
        result = newAge;
        result += (2*variation) * (rand() / RAND_MAX);
        result -= variation;
        particles[i]->age = result;
    }
}

void VRParticles::setLifetime(int newLifetime, int variation) {
    int i, result;

    BLock lock(mtx());
    for (i=0; i<N; i++) {
        result = newLifetime;
        result += (2*variation) * (rand() / RAND_MAX);
        result -= variation;
        particles[i]->lifetime = result;
    }
}

int VRParticles::spawnCuboid(Vec3f center, Vec3f size, float distance) {
    if (distance == 0.0) distance = this->particles[0]->radius;

    int numX = size[0] / distance;
    int numY = size[1] / distance;
    int numZ = size[2] / distance;
    int spawned = 0;
    bool done = false;
    int i,j,k;
    int posX, posY, posZ;
    btVector3 pos;

    //auto btcenter = toBtVector3(center - size*0.5); // TODO -> refactor and test the whole function
    auto btcenter = toBtVector3(center);
    {
        BLock lock(mtx());
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
                        pos += btcenter;
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
    int N = particles.size();
    if (to > N || from > N || from > to) {
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
    auto e = Emitter::create();
    e->set(world, p, base, dir, interval, this->collideWithSelf);
    e->setLoop(loop);
    this->emitters[e->id] = e; //store emitters

    setFunctions(from, to);
    {
        BLock lock(mtx());
        e->setActive(true);
    }
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
    this->from = from;
    this->to = to;
    VRScenePtr scene = VRScene::getCurrent();
    scene->dropUpdateFkt(fkt);
    fkt = VRFunction<int>::create("particles_update", boost::bind(&VRParticles::updateParticles, this,from,to));
    scene->addUpdateFkt(fkt);
}

void VRParticles::disableFunctions() {
    VRScenePtr scene = VRScene::getCurrent();
    if (scene) scene->dropUpdateFkt(fkt);
}
