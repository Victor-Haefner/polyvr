#include "VRPhysicsManager.h"

#include <btBulletDynamicsCommon.h>
#include <iostream>
#include <OpenSG/OSGGLUT.h>
#include <OpenSG/OSGTriangleIterator.h>
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


VRPhysicsManager::VRPhysicsManager() {
    // Build the broadphase
    broadphase = new btDbvtBroadphase();

    // Set up the collision configuration and dispatcher
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);

    // The actual physics solver
    solver = new btSequentialImpulseConstraintSolver;

    // The world.
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0,-10,0));

    updatePhysicsFkt = new VRFunction<int>("PhysicsUpdate", boost::bind(&VRPhysicsManager::updatePhysics, this));

    cout << "Init VRPhysicsManager" << endl;
}

VRPhysicsManager::~VRPhysicsManager() {
    return;

    //cleanup in the reverse order of creation/initialization

    //remove the rigidbodies from the dynamics world and delete them
    for (int i=dynamicsWorld->getNumCollisionObjects()-1; i>=0 ;i--)
    {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getMotionState())
        {
            delete body->getMotionState();
        }
        dynamicsWorld->removeCollisionObject( obj );
        delete obj;
    }

    //delete collision shapes
    for (int j=0;j<collisionShapes.size();j++)
    {
        btCollisionShape* shape = collisionShapes[j];
        collisionShapes[j] = 0;
        delete shape;
    }

    // Clean up behind ourselves like good little programmers
    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;

}

void VRPhysicsManager::updatePhysics() {

    if (dynamicsWorld == 0) return;

    static int t_last = glutGet(GLUT_ELAPSED_TIME);
    int t = glutGet(GLUT_ELAPSED_TIME);
    dynamicsWorld->stepSimulation((t-t_last)*0.001, 30);
    collectCollisionPoints();
    t_last = t;

    //print positions of all objects
    for (int j=dynamicsWorld->getNumCollisionObjects()-1; j>=0 ;j--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[j];
        body = btRigidBody::upcast(obj);
        if (body && body->getMotionState() && OSGobjs.count(body) == 1) OSGobjs[body]->updateFromBullet();
    }
}

void VRPhysicsManager::physicalize(VRTransform* obj) {
    OSGobjs[obj->getPhysics()->obj()] = obj;
}

void VRPhysicsManager::setGravity(Vec3f g) {
    dynamicsWorld->setGravity(btVector3(g[0],g[1],g[2]));
}

void VRPhysicsManager::collectCollisionPoints() {
    btVector3 p1, p2, n;
    Vec3f p;

    //btCollisionObject *o1, *o2;
    collisionPoints.clear();

	for (int i=0; i<dynamicsWorld->getDispatcher()->getNumManifolds(); i++) {
		btPersistentManifold* contactManifold = dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
		//o1 = static_cast<btCollisionObject*>(contactManifold->getBody0());
		//o2 = static_cast<btCollisionObject*>(contactManifold->getBody1());

		for (int j=0; j<contactManifold->getNumContacts(); j++) {
			btManifoldPoint& pt = contactManifold->getContactPoint(j);
			if (pt.getDistance()<0.f) {
				p1 = pt.getPositionWorldOnA();
				p2 = pt.getPositionWorldOnB();
				n = pt.m_normalWorldOnB;

                collisionPoints.push_back( Vec3f(p1[0], p1[1], p1[2]) );
			}
		}
	}
}

btDiscreteDynamicsWorld* VRPhysicsManager::bltWorld() { return dynamicsWorld; }

vector<Vec3f>& VRPhysicsManager::getCollisionPoints() {
    return collisionPoints;
}


OSG_END_NAMESPACE;
