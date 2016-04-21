#include "VREmitter.h"

using namespace std;
using namespace OSG;


typedef boost::recursive_mutex::scoped_lock BLock;

boost::recursive_mutex& Emitter::mtx() {
    auto scene = OSG::VRSceneManager::getCurrent();
    if (scene) return scene->physicsMutex();
    else {
        static boost::recursive_mutex m;
        return m;
    };
}


// initialize static variable
int Emitter::newId = 0;

Emitter::Emitter(btDiscreteDynamicsWorld* world, vector<Particle*> particlesV, btVector3 pos, btVector3 dir, int emit_freq, bool collide) {
    this->particles = particlesV;
    this->position = pos;
    this->direction = dir;
    this->interval = emit_freq;
    this->world = world;
    this->collideSelf = collide;

    id = newId;
    newId++;

    p_num = 0;
    fkt = VRFunction<int>::create("particles_update", boost::bind(&Emitter::emitterLoop, this));
}

Emitter::~Emitter() {
    setActive(false);
    VRScenePtr scene = VRSceneManager::getCurrent();
    scene->dropUpdateFkt(fkt);
}

void Emitter::setActive(bool activate) {
    VRScenePtr scene = VRSceneManager::getCurrent();
    if (activate && !active) {
        scene->addUpdateFkt(fkt);
        active = true;
    } else if (!activate) {
        scene->dropUpdateFkt(fkt);
        active = false;
    }
}

void Emitter::setLoop(bool activate) {
    this->loop = activate;
    if (activate) {
        //BLock lock(mtx());
        VRScenePtr scene = VRSceneManager::getCurrent();
        scene->addUpdateFkt(fkt);
    } else {
        //BLock lock(mtx());
        VRScenePtr scene = VRSceneManager::getCurrent();
        scene->dropUpdateFkt(fkt);
    }
}

/**
 * Emits one particle at a time
 */
void Emitter::emitterLoop() {
    timer++;
    if (timer == 1) {
        Particle* p = particles[this->p_num];
        {
            BLock lock(mtx());
            p->spawnAt(position, this->world, this->collideSelf);
            p->setActive(true);
            p->body->setLinearVelocity(direction);
        }
        this->p_num++;
        if (p_num >= particles.size()) {
            if (loop) {
                //emit_i = emit_from;
                // TODO enable looped emitter by extending p->spawnAt()
                this->setActive(false);
            } else this->setActive(false);
        }
    } else if (timer >= interval-1) timer = 0;
}
