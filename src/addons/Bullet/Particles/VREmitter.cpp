#include "VREmitter.h"
#include "core/scene/VRScene.h"

using namespace std;
using namespace OSG;


typedef boost::recursive_mutex::scoped_lock BLock;

boost::recursive_mutex& Emitter::mtx() {
    static boost::recursive_mutex m;
    auto scene = OSG::VRScene::getCurrent();
    if (scene) return scene->physicsMutex();
    else return m;
}

Emitter::Emitter() {}

void Emitter::set(btDiscreteDynamicsWorld* world, vector<Particle*> particlesV, btVector3 pos, btVector3 dir, int emit_freq, bool collide) {
    this->particles = particlesV;
    this->position = pos;
    this->direction = dir;
    this->interval = emit_freq;
    this->world = world;
    this->collideSelf = collide;

    static int ID = 0;
    id = ID; ID++;
    fkt = VRFunction<int>::create("particles_update", boost::bind(&Emitter::emitterLoop, this));
}

Emitter::~Emitter() {
    setActive(false);
    VRScenePtr scene = VRScene::getCurrent();
    scene->dropUpdateFkt(fkt);
}

shared_ptr<Emitter> Emitter::create() { return shared_ptr<Emitter>( new Emitter() ); }

void Emitter::setActive(bool activate) {
    VRScenePtr scene = VRScene::getCurrent();
    if (!scene) return;
    if (activate && !active) scene->addUpdateFkt(fkt);
    if (!activate) scene->dropUpdateFkt(fkt);
    active = activate;
}

void Emitter::setLoop(bool activate) {
    loop = activate;
    auto scene = VRScene::getCurrent();
    if (!scene) return;
    if (activate) scene->addUpdateFkt(fkt);
    else scene->dropUpdateFkt(fkt);
}

void Emitter::emitterLoop() {
    if (timer == 0) {
        Particle* p = particles[p_num];
        {
            BLock lock(mtx());
            p->spawnAt(position, world, collideSelf);
            p->setActive(true);
            p->body->setLinearVelocity(direction);
        }
        p_num++;
        if (p_num >= particles.size()) {
            cout << "Emitter::emitterLoop " << loop << endl;
            if (loop) p_num = 0;
            else setActive(false);
        }
    }

    timer++;
    if (timer >= interval-1) timer = 0;
}
