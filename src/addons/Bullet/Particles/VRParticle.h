#ifndef VRPARTICLE_H_INCLUDED
#define VRPARTICLE_H_INCLUDED

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGGeoProperties.h>
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>



using namespace std;
OSG_BEGIN_NAMESPACE;

struct Particle {
    float mass = 0.01; // TODO unit is ???
    float radius = 0.01; // unit is meter
    unsigned int age = 0; // current age
    unsigned int lifetime = 0; // max age. 0 means immortal.

    btRigidBody* body = 0;
    btCollisionShape* shape = 0;
    btDefaultMotionState* motionState = 0;
    int collisionGroup = 1;
    int collisionMask = 1;

    void spawnAt(btVector3 v, btDiscreteDynamicsWorld* world = 0) {
        if (world == 0) return;
        // TODO if Particles shall be spawned a second time, handle it.
        // TODO most of this should be done in constructor. Only the last line should be here if possible.

        btTransform t;
        t.setOrigin(btVector3(v.x(),v.y(),v.z()));
        motionState = new btDefaultMotionState(t);

        shape = new btSphereShape(radius);

        btVector3 inertiaVector(0,0,0);
        shape->calculateLocalInertia(mass, inertiaVector);
        btRigidBody::btRigidBodyConstructionInfo rbInfo( mass, motionState, shape, inertiaVector );

        body = new btRigidBody(rbInfo);
        body->setActivationState(ACTIVE_TAG);

        world->addRigidBody(body, collisionGroup, collisionMask);
    }

    Particle(btDiscreteDynamicsWorld* world = 0) {}

    ~Particle() {
        VRScenePtr scene = VRSceneManager::getCurrent();
        if (scene && body) { scene->bltWorld()->removeRigidBody(body); }
        if (body) { delete body; }
        if (shape) { delete shape; }
        if (motionState) { delete motionState; }
    }
};

struct SphParticle : public Particle {
    float sphArea = 3 * radius;
    float sphDensity = 0.1;
    float sphPressure = 1.0;
    btVector3 sphPressureForce;
    btVector3 sphViscosityForce;

    SphParticle(btDiscreteDynamicsWorld* world = 0) {
        sphPressureForce.setZero();
        sphViscosityForce.setZero();
    }

    string toString_Position(string append) {
        btVector3 pos = body->getWorldTransform().getOrigin();
        return "(" + to_string(pos[0]) + ", " + to_string(pos[1]) + ", " + to_string(pos[2]) + ")"
                + append;
    }

    string toString_PressureFoo(string append) {
        btVector3 pF = sphPressureForce;
        string dens = to_string(sphDensity);
        string press = to_string(sphPressure);
        return "d(" + dens + "), p(" + press + "), pForce("
                + to_string(pF[0]) + ", " + to_string(pF[1]) + ", " + to_string(pF[2]) + ")"
                + append;
    }
};
OSG_END_NAMESPACE;
#endif // VRPARTICLE_H_INCLUDED
