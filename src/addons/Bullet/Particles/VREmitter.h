#ifndef VREMITTER_H_INCLUDED
#define VREMITTER_H_INCLUDED

#include "VRParticle.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGGeoProperties.h>
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>


using namespace std;
OSG_BEGIN_NAMESPACE;

class Emitter {

public:
    static int newId;
    int id;

    Emitter();
    Emitter(btDiscreteDynamicsWorld* world, vector<Particle*> particlesV, btVector3 pos, btVector3 dir, int emit_freq);
    ~Emitter();

    void setActive(bool activate);
    void setLoop(bool activate);

private:
    vector<Particle*> particles;
    btVector3 position;
    btVector3 direction;
    int interval = 60;
    int counter = 0;
    int i = 0;
    bool loop = false;
    bool active = false;

    void emitterLoop();
};
OSG_END_NAMESPACE;
#endif // VREMITTER_H_INCLUDED
