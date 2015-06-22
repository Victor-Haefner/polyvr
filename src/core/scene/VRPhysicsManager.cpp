#include "VRPhysicsManager.h"

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <BulletCollision/CollisionShapes/btConvexPolyhedron.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
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
#include "core/objects/geometry/VRPrimitive.h"

#include <unistd.h>

#define PHYSICS_THREAD_TIMESTEP_MS 2

typedef boost::recursive_mutex::scoped_lock MLock;

OSG_BEGIN_NAMESPACE;
using namespace std;


VRPhysicsManager::VRPhysicsManager() {
    MLock lock(mtx);
    // Build the broadphase
    broadphase = new btDbvtBroadphase();

    // Set up the collision configuration && dispatcher
    collisionConfiguration = new btSoftBodyRigidBodyCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);

    // The actual physics solver
    solver = new btSequentialImpulseConstraintSolver;

    // The world.
    dynamicsWorld = new btSoftRigidDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0,-10,0));
    dynamicsWorld->getPairCache()->setInternalGhostPairCallback( new btGhostPairCallback() );

    //The soft world attributes
    softBodyWorldInfo =     &(dynamicsWorld->getWorldInfo());
   	softBodyWorldInfo->m_dispatcher = dispatcher;
   	softBodyWorldInfo->m_broadphase = broadphase;
	softBodyWorldInfo->m_gravity.setValue(0,10,0);
    softBodyWorldInfo->air_density	= (btScalar)1.2;
    softBodyWorldInfo->water_density	= 0;
    softBodyWorldInfo->water_offset	= 0;
    softBodyWorldInfo->water_normal	= btVector3(0,0,0);


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

boost::recursive_mutex& VRPhysicsManager::physicsMutex() { return mtx; }

long long VRPhysicsManager::getTime() { // time in seconds
    return 1000*glutGet(GLUT_ELAPSED_TIME);
    //return 1e6*clock()/CLOCKS_PER_SEC; // TODO
}
btSoftBodyWorldInfo* VRPhysicsManager::getSoftBodyWorldInfo() {return softBodyWorldInfo;}

void VRPhysicsManager::prepareObjects() {
    for (auto o : OSGobjs) o.second->getPhysics()->prepareStep();
}

void VRPhysicsManager::updatePhysics(VRThread* thread) {
    if (dynamicsWorld == 0) return;
    long long dt,t0,t1,t2,t3;
    t0 = thread->t_last;
    t1 = getTime();
    thread->t_last = t1;
    dt = t1-t0;

    {
        MLock lock(mtx);
        prepareObjects();
        for (auto f : updateFktsPre) (*f)(0);
        dynamicsWorld->stepSimulation(1e-6*dt, 30);
        for (auto f : updateFktsPost) (*f)(0);
    }

    t2 = getTime();
    dt = t2-t1;

    //sleep up to 500 fps
    if (dt < PHYSICS_THREAD_TIMESTEP_MS * 1000) usleep(PHYSICS_THREAD_TIMESTEP_MS * 1000 -dt);
    t3 = getTime();

    MLock lock(mtx);
    fps = 1e6/(t3-t1);

}

void VRPhysicsManager::addPhysicsUpdateFunction(VRFunction<int>* fkt, bool after) {
    MLock lock(mtx);
    if (after) updateFktsPost.push_back(fkt);
    else updateFktsPre.push_back(fkt);
}
void VRPhysicsManager::dropPhysicsUpdateFunction(VRFunction<int>* fkt, bool after) {
    MLock lock(mtx);
    vector<VRFunction<int>* >* fkts = after ? &updateFktsPost : &updateFktsPre;
    for(int i = 0; i < fkts->size() ; i++) {
            if(fkts->at(i) == fkt) {fkts->erase(fkts->begin() + i);return;}
    }
 }

void VRPhysicsManager::updatePhysObjects() {
    //mtx.try_lock();
    MLock lock(mtx);
    VRGlobals::get()->PHYSICS_FRAME_RATE = fps;

    for (auto o : OSGobjs) {
        if (o.second->getPhysics()->isGhost()) o.second->updatePhysics();
    }

    collectCollisionPoints();

    for (int j=dynamicsWorld->getNumCollisionObjects()-1; j>=0 ;j--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[j];
        body = btRigidBody::upcast(obj);
        if (body && body->getMotionState() && OSGobjs.count(body) == 1) OSGobjs[body]->updateFromBullet();
    }

    //the soft bodies
    btSoftBodyArray arr = dynamicsWorld->getSoftBodyArray();
    //Patches
    VRTransform* soft_trans;
    btSoftBody* patch;
    for(int i = 0; i < arr.size() ;i++) { //for all soft bodies
        soft_trans = OSGobjs[arr[i]]; //get the corresponding transform to this soft body
        patch = arr[i];   //the soft body
        vector<OSG::VRObject*> geos = soft_trans->getObjectListByType("Geometry"); //get all geometries underlying this transform
        for (unsigned int j=0; j<geos.size(); j++) { //for each geometry
            OSG::VRGeometry* geo = (OSG::VRGeometry*)geos[j];
            if (geo == 0) continue;


            //render the visual
            OSG::VRGeometry* visualgeo = physics_visuals[patch];

            btSoftBody::tNodeArray&   nodes(patch->m_nodes);
            btSoftBody::tFaceArray&   faces(patch->m_faces);
            btSoftBody::tLinkArray&   links(patch->m_links);
            GeoPnt3fPropertyRecPtr      visualpos = GeoPnt3fProperty::create();
            GeoUInt32PropertyRecPtr visualinds = GeoUInt32Property::create();
            GeoVec3fPropertyRecPtr visualnorms = GeoVec3fProperty::create();

            for (unsigned int i = 0; i<nodes.size(); i++) { //go through the nodes and copy positions to mesh positionarray
                    Vec3f p = VRPhysics::toVec3f(nodes[i].m_x);
                    OSG::Vec3f tmp;
                    visualpos->addValue(p);
                    Vec3f n = VRPhysics::toVec3f(nodes[i].m_n);
                    visualnorms->addValue( n );
            }

            for(int j=0;j<faces.size();++j)
           {
              btSoftBody::Node*   node_0=faces[j].m_n[0];
              btSoftBody::Node*   node_1=faces[j].m_n[1];
              btSoftBody::Node*   node_2=faces[j].m_n[2];
             const int indices[]={   int(node_0-&nodes[0]),
                                      int(node_1-&nodes[0]),
                                      int(node_2-&nodes[0])};
                visualinds->addValue(indices[0]);
                visualinds->addValue(indices[1]);
                visualinds->addValue(indices[2]);
           }
            GeoUInt32PropertyRecPtr vtypes = GeoUInt32Property::create();
            GeoUInt32PropertyRecPtr vlens = GeoUInt32Property::create();
            vtypes->addValue(GL_TRIANGLES);
            vlens->addValue(faces.size());
            vtypes->addValue(GL_LINES);
            vlens->addValue(links.size());

            visualgeo->setType(GL_TRIANGLES    );
            visualgeo->setPositions(visualpos);
            visualgeo->setIndices(visualinds);
            visualgeo->setNormals(visualnorms);


            //only for plane soft bodies : directly apply nodes to vertices of geometry model
            if(geo->getPrimitive()->getType() == "Plane") { //render it correctly
                VRPlane* prim = (VRPlane*)geo->getPrimitive();
                Matrix m = geo->getWorldMatrix();
                GeoPnt3fPropertyRecPtr      positions = GeoPnt3fProperty::create();
                GeoVec3fPropertyRecPtr norms = GeoVec3fProperty::create();
                GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();

                for (unsigned int i = 0; i<nodes.size(); i++) { //go through the nodes and copy positions to mesh positionarray
                    Vec3f p = VRPhysics::toVec3f(nodes[i].m_x);
                    OSG::Vec3f tmp;
                    positions->addValue(p);
                    Vec3f n = VRPhysics::toVec3f(nodes[i].m_n);
                    norms->addValue( n );
                }
                geo->setPositions(positions);
                geo->setNormals(norms);
           }
        }
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
            const unsigned int* bt_inds =   hull.getIndexPointer();
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
    if (bdy == 0) return;
    cout << " with bt_body " << (bdy == 0) << endl;
    OSGobjs[bdy] = obj;
    physics_visuals_to_update.push_back(bdy);

    if (physics_visuals.count(bdy) == 0) { // TODO: refactor this
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

    if (physics_visuals.count(bdy)) { // TODO: refactor this
        delete physics_visuals[bdy];
        physics_visuals.erase(bdy);
    }
}

void VRPhysicsManager::setGravity(Vec3f g) {
    MLock lock(mtx);
    dynamicsWorld->setGravity(btVector3(g[0],g[1],g[2]));
}

void VRPhysicsManager::collectCollisionPoints() {
    MLock lock(mtx);
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

btSoftRigidDynamicsWorld* VRPhysicsManager::bltWorld() { return dynamicsWorld; }

vector<Vec3f>& VRPhysicsManager::getCollisionPoints() {
    return collisionPoints;
}


OSG_END_NAMESPACE;
