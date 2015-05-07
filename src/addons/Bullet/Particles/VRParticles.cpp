#include "VRParticles.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"

#include <btBulletDynamicsCommon.h>

typedef boost::recursive_mutex::scoped_lock BLock;

using namespace std;
using namespace OSG;

boost::recursive_mutex& mtx() {
    auto scene = OSG::VRSceneManager::getCurrent();
    if (scene) return scene->physicsMutex();
    else {
        static boost::recursive_mutex m;
        return m;
    };
}

struct OSG::Particle {
    btRigidBody* body = 0;
    btCollisionShape* shape = 0;
    btDefaultMotionState* motionState = 0;
    float radius = 0.01;
    int collisionGroup = 1;
    int collisionMask = 1;
    float mass = 1.0;

    //Particle(btDiscreteDynamicsWorld* world);
    Particle(btDiscreteDynamicsWorld* world = 0) {
        if (world == 0) return;

        float x = 0.1*float(rand())/RAND_MAX;
        float y = 0.1*float(rand())/RAND_MAX;
        float z = 0.1*float(rand())/RAND_MAX;

        btTransform t;
        t.setOrigin(btVector3(x,y,z));
        motionState = new btDefaultMotionState(t);

        shape = new btSphereShape(radius);

        btVector3 inertiaVector(0,0,0);
        shape->calculateLocalInertia(mass, inertiaVector);
        btRigidBody::btRigidBodyConstructionInfo rbInfo( mass, motionState, shape, inertiaVector );

        body = new btRigidBody(rbInfo);
        body->setActivationState(ACTIVE_TAG);

        world->addRigidBody(body, collisionGroup, collisionMask);
    }

    ~Particle() {
        btDiscreteDynamicsWorld* world = 0;
        VRScene* scene = VRSceneManager::getCurrent();
        if (scene) scene->bltWorld()->removeRigidBody(body);
        delete body;
        delete shape;
        delete motionState;
    }
};

Vec3f toVec3f(btVector3 v) { return Vec3f(v[0], v[1], v[2]); }
btVector3 toBtVector3(Vec3f v) { return btVector3(v[0], v[1], v[2]); }

VRParticles::VRParticles() : VRGeometry("particles") {
    N = 500;

    // physics
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene) world = scene->bltWorld();
    particles.resize(N, 0);

    {
        BLock lock(mtx());
        for(int i=0;i<N;i++) particles[i] = new Particle(world);
    }

    // material
    mat = new VRMaterial("particles");
    mat->setDiffuse(Vec3f(0,0,1));
    mat->setPointSize(5);
    mat->setLit(false);

    // geometry
    GeoUInt32PropertyRecPtr     Length = GeoUInt32Property::create();
    GeoUInt32PropertyRecPtr     inds = GeoUInt32Property::create();
    pos = GeoPnt3fProperty::create();

    Length->addValue(N);

    for(int i=0;i<N;i++) pos->addValue(Pnt3f(0,0,0));
    for(int i=0;i<N;i++) inds->addValue(i);

    setType(GL_POINTS);
    setLengths(Length);
    setPositions(pos);
    setIndices(inds);
    setMaterial(mat);

    // update loop
    fkt = new VRFunction<int>("particles_update", boost::bind(&VRParticles::update, this,0,-1));
    scene->addUpdateFkt(fkt);
}

VRParticles::~VRParticles() {
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene) scene->dropUpdateFkt(fkt);
    delete mat;

    BLock lock(mtx());
    for (int i=0;i<N;i++) delete particles[i];
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
