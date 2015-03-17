#include "VRPhysicsManager.h"

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <BulletCollision/CollisionShapes/btConvexPolyhedron.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <iostream>
#include <OpenSG/OSGGLUT.h>
#include <OpenSG/OSGTriangleIterator.h>
#include <OpenSG/OSGNode.h>
#include <OpenSG/OSGGeometry.h>
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRVisualLayer.h"
#include "VRThreadManager.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


VRPhysicsManager::VRPhysicsManager() {
    // Build the broadphase
    broadphase = new btDbvtBroadphase();

    // Set up the collision configuration && dispatcher
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);

    // The actual physics solver
    solver = new btSequentialImpulseConstraintSolver;

    // The world.
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0,-10,0));
    dynamicsWorld->getPairCache()->setInternalGhostPairCallback( new btGhostPairCallback() );

    updatePhysObjectsFkt = new VRFunction<int>("Physics object update", boost::bind(&VRPhysicsManager::updatePhysObjects, this));
    updatePhysicsFkt = new VRFunction<VRThread*>("Physics update", boost::bind(&VRPhysicsManager::updatePhysics, this, _1));

    physics_visual_layer = new VRVisualLayer("physics", "physics.png");

    phys_mat = new VRMaterial("phys_mat");
    phys_mat->setLit(false);
    phys_mat->setDiffuse(Vec3f(0.8,0.8,0.4));
    phys_mat->setTransparency(0.4);

    cout << "Init VRPhysicsManager" << endl;
}

VRPhysicsManager::~VRPhysicsManager() {
    return; // TODO

    //remove the rigidbodies from the dynamics world && delete them
    for (int i=dynamicsWorld->getNumCollisionObjects()-1; i>=0 ;i--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getMotionState()) delete body->getMotionState();
        dynamicsWorld->removeCollisionObject( obj );
        delete obj;
    }

    //delete collision shapes
    for (int j=0;j<collisionShapes.size();j++) {
        btCollisionShape* shape = collisionShapes[j];
        collisionShapes[j] = 0;
        delete shape;
    }

    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;
}

void VRPhysicsManager::updatePhysics(VRThread* thread) {
    if (dynamicsWorld == 0) return;

    static int t_last = glutGet(GLUT_ELAPSED_TIME);
    int t = glutGet(GLUT_ELAPSED_TIME);
    dynamicsWorld->stepSimulation((t-t_last)*0.001, 30);
    t_last = t;
}

void VRPhysicsManager::updatePhysObjects() {
    for (auto o : OSGobjs) {
        if (o.second->getPhysics()->isGhost()) o.second->updatePhysics();
    }

    collectCollisionPoints();

    for (int j=dynamicsWorld->getNumCollisionObjects()-1; j>=0 ;j--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[j];
        body = btRigidBody::upcast(obj);
        if (body && body->getMotionState() && OSGobjs.count(body) == 1) OSGobjs[body]->updateFromBullet();
    }

    // update physics visualisation shapes
    for (auto v : physics_visuals_to_update) {
        if (physics_visuals.count(v) == 0) continue;
        VRGeometry* geo = physics_visuals[v];
        //cout << "try " << v << " " << geo << endl;
        btCollisionShape* shape = v->getCollisionShape();
        int stype = shape->getShapeType();

        // 4 : convex
        // 8 : sphere
        // 0 : box
        // 21 : concave

        if (stype == 8) { // sphere
            btSphereShape* sshape = (btSphereShape*)shape;
            btScalar radius = sshape->getRadius();
            stringstream params;
            params << radius*1.01 << " 2";
            geo->setPrimitive("Sphere", params.str());
        }

        if (stype == 0) { // box
            btBoxShape* bshape = (btBoxShape*)shape;
            btVector4 plane;
            Vec3f dim;
            bshape->getPlaneEquation(plane, 0); dim[0] = 2*(abs(plane[3]) + shape->getMargin());
            bshape->getPlaneEquation(plane, 2); dim[1] = 2*(abs(plane[3]) + shape->getMargin());
            bshape->getPlaneEquation(plane, 4); dim[2] = 2*(abs(plane[3]) + shape->getMargin());
            stringstream params;
            params << dim[0]*1.01 << " " << dim[1]*1.01 << " " << dim[2]*1.01 << " 1 1 1";
            geo->setPrimitive("Box", params.str());
        }

        if (stype == 4) { // convex
            btConvexHullShape* cshape = (btConvexHullShape*)shape;
            btShapeHull hull(cshape);
            hull.buildHull(cshape->getMargin());

            int Ni = hull.numIndices();
            int Nv = hull.numVertices();
            const unsigned int* bt_inds = hull.getIndexPointer();
            const btVector3* verts = hull.getVertexPointer();

            GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
            GeoVec3fPropertyRecPtr norms = GeoVec3fProperty::create();
            GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();

            for (int i=0; i<Ni; i++) inds->addValue( bt_inds[i] );
            for (int i=0; i<Nv; i++) {
                Vec3f p = VRPhysics::toVec3f(verts[i]);
                pos->addValue( p );
                p.normalize();
                norms->addValue( p );
            }

            geo->setType(GL_TRIANGLES);
            geo->setPositions(pos);
            geo->setNormals(norms);
            geo->setIndices(inds);
        }

        geo->setMaterial(phys_mat);
    }
    physics_visuals_to_update.clear();

    // update physics visualisation
    if (physics_visual_layer->getVisibility()) {
        for (auto obj : physics_visuals) {
            VRGeometry* geo = obj.second; // transfer transformation
            btTransform trans = obj.first->getWorldTransform();
            geo->setMatrix( VRPhysics::fromBTTransform( trans ) );
        }
    }
}

void VRPhysicsManager::physicalize(VRTransform* obj) {
    //cout << "physicalize transform: " << obj;
    btCollisionObject* bdy = obj->getPhysics()->getCollisionObject();
    //cout << " with bt_body " << bdy << endl;
    if (bdy == 0) return;

    OSGobjs[bdy] = obj;
    physics_visuals_to_update.push_back(bdy);

    if (physics_visuals.count(bdy) == 0) {
        VRGeometry* pshape = new VRGeometry("phys_shape");
        physics_visuals[bdy] = pshape;
        physics_visual_layer->addObject(pshape);
    }
}

void VRPhysicsManager::unphysicalize(VRTransform* obj) {
    //cout << "unphysicalize transform: " << obj;
    btCollisionObject* bdy = obj->getPhysics()->getCollisionObject();
    //cout << " with bt_body " << bdy << endl;
    if (bdy == 0) return;
    if (OSGobjs.count(bdy)) OSGobjs.erase(bdy);
    if (physics_visuals.count(bdy)) {
        delete physics_visuals[bdy];
        physics_visuals.erase(bdy);
    }
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
