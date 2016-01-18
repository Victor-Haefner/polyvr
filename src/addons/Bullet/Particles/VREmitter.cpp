#include "VREmitter.h"

using namespace std;
using namespace OSG;


// initialize const static variable
int Emitter::newId = 0;

Emitter::Emitter(btDiscreteDynamicsWorld* world, vector<Particle*> particlesV, btVector3 pos, btVector3 dir, int emit_freq) {
    particles = particlesV;
    position = pos;
    direction = dir;
    interval = emit_freq;
    id = newId;
    newId++;
}

Emitter::~Emitter() {
    setActive(false);
}

void Emitter::setActive(bool activate) {
    VRScenePtr scene = VRSceneManager::getCurrent();
    if (activate && !active) {
        //fkt = VRFunction<int>::create("particles_update", boost::bind(&VRParticles::update, this,from,to));
        //scene->addUpdateFkt(fkt);
        active = true;
    } else {
        //scene->dropUpdateFkt(fkt);
        active = false;
    }
}

void Emitter::setLoop(bool activate) {
    this->loop = activate;
}

/**
 * Emits one particle at a time
 */
// void Emitter::emitterLoop() {
//     if (emit_counter == 1) {
//         Particle* p = particles[emit_i];
//         p->spawnAt(emit_base, this->world, this->collideWithSelf);
//         printf("Emitter: Particle emitted\n");
//         p->body->applyCentralForce(emit_dir);
//         emit_i++;
//         emit_counter++;
//         if (emit_i == emit_to) {
//             if (emit_loop) {
//                 //emit_i = emit_from;
//                 // TODO enable looped emitter by extending p->spawnAt()
//                 this->disableEmitter();
//             } else {
//                 this->disableEmitter();
//             }
//         }
//     } else if (emit_counter >= emit_interval-1) {
//         emit_counter = 0;
//     } else {
//         emit_counter++;
//     }
// }
