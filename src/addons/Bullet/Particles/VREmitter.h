#ifndef VREMITTER_H_INCLUDED
#define VREMITTER_H_INCLUDED

#include "VRParticle.h"

#include <OpenSG/OSGConfig.h>
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>


using namespace std;
OSG_BEGIN_NAMESPACE;

class Emitter {

public:
    int id;

    Emitter();
    ~Emitter();

    static shared_ptr<Emitter> create();

    void set(btDiscreteDynamicsWorld* world, vector<Particle*> particlesV, btVector3 pos, btVector3 dir, int emit_freq, bool collide=false);

    void setActive(bool activate);
    void setLoop(bool activate);

protected:
    btDiscreteDynamicsWorld* world = 0;
    vector<Particle*> particles;
    btVector3 position;
    btVector3 direction;
    unsigned int interval = 60;
    unsigned int timer = 0;
    unsigned int p_num = 0;
    bool loop = false;
    bool active = false;
    bool collideSelf = false;
    VRUpdateCbPtr fkt;

    void emitterLoop();
    boost::recursive_mutex& mtx();
};
OSG_END_NAMESPACE;
#endif // VREMITTER_H_INCLUDED
