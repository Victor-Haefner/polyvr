#include "VRParticles.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"

#include <cmath> /* cbrtf() */
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>

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
        VRScene* scene = VRSceneManager::getCurrent();
        if (scene) scene->bltWorld()->removeRigidBody(body);
        delete body;
        delete shape;
        delete motionState;
    }
};

Vec3f toVec3f(btVector3 v) { return Vec3f(v[0], v[1], v[2]); }
btVector3 toBtVector3(Vec3f v) { return btVector3(v[0], v[1], v[2]); }

VRParticles::VRParticles(int particleAmount) : VRGeometry("particles") {
    N = particleAmount;
    particles.resize(N, 0);

    // physics
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene) world = scene->bltWorld();

    {
        BLock lock(mtx());
        for(int i=0;i<N;i++) particles[i] = new Particle();
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
}

VRParticles::~VRParticles() {
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene) scene->dropUpdateFkt(fkt);
    delete mat;

    {
        BLock lock(mtx());
        for (int i=0;i<N;i++) delete particles[i];
    }
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

float VRParticles::getMaxRadius() {
    int i;
    float maximum = 0;
    {
        BLock lock(mtx());
        for (i=0; i<N; i++) {
            if (maximum < particles[i]->radius) maximum = particles[i]->radius;
        }
    }

    return maximum;
}

int VRParticles::spawnRect(Vec3f base, ArgType type, float a, float b, float c) {
    VRScene* scene = VRSceneManager::getCurrent();
    // if (scene) world = scene->bltWorld();

    float radius;
    int required;
    radius = getMaxRadius();

    switch (type) {
        case SIZE:
            break;
        case NOTHING: // default case: spawn 10 Liter!
            a = 10;
            type = LITER;
            // go on with LITER (therefore no break;)
        case LITER:
            a = b = c = cbrtf(a);
            break;
    }
    required = (int)  ( (a/radius) * (b/radius) * (c/radius) );
    if (required > N) required = N;

    // now, randomly place particles in rectangle.
    // TODO place more intelligent (grid, not random) to avoid problems
    btVector3 v;
    float y,x,z; x=y=z=0;
    int i;
    {
        BLock lock(mtx());
        for (i=0; i<required; i++)
        {
            // FIXME: for some reason, base is still not the middle point between all particles.
            v.setZero();
            x = (a*float(rand())/RAND_MAX);
            x = x - (x/2);
            v.setX (x);

            y = (b*float(rand())/RAND_MAX);
            y = y - (y/2);
            v.setY (y);

            z = (c*float(rand())/RAND_MAX);
            z = z - (z/2);
            v.setZ (z);

            v += toBtVector3(base);
            particles[i]->spawnAt(v, this->world);
        }
    // activate update loop
    scene->addUpdateFkt(fkt);
    }
    return required;
}
