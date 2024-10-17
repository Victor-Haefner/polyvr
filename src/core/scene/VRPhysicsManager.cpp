#include "VRPhysicsManager.h"

#include "core/utils/VRMutex.h"
#include <btBulletDynamicsCommon.h>
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
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRVisualLayer.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/VRProfiler.h"
#include "VRThreadManager.h"
#include "core/objects/geometry/VRPrimitive.h"

#include "core/utils/Thread.h"

#define PHYSICS_THREAD_TIMESTEP_MS 2



OSG_BEGIN_NAMESPACE;
using namespace std;


VRPhysicsManager::VRPhysicsManager() {
    mtx = new VRMutex();
    VRLock lock(*mtx);
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
	softBodyWorldInfo->m_gravity.setValue(0,-10,0);
    softBodyWorldInfo->air_density	= (btScalar)1.2;
    softBodyWorldInfo->water_density	= 0;
    softBodyWorldInfo->water_offset	= 0;
    softBodyWorldInfo->water_normal	= btVector3(0,0,0);


    updatePhysObjectsFkt = VRUpdateCb::create("Physics object update", bind(&VRPhysicsManager::updatePhysObjects, this));
    updatePhysicsFkt = VRThreadCb::create("Physics update", bind(&VRPhysicsManager::updatePhysics, this, _1));

    physics_visual_layer = VRVisualLayer::getLayer("Physics", "physics.png", 1);

    phys_mat = VRMaterial::get("phys_mat");
    phys_mat->setLit(false);
    phys_mat->setDiffuse(Color3f(0.8,0.8,0.4));
    phys_mat->setTransparency(0.4);

    cout << "Init VRPhysicsManager" << endl;
}

VRPhysicsManager::~VRPhysicsManager() {
    //for (auto o : OSGobjs) unphysicalize(o.second);

    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;
    delete mtx;
}

VRMutex& VRPhysicsManager::physicsMutex() { return *mtx; }

VRVisualLayerPtr VRPhysicsManager::getVisualLayer() { return physics_visual_layer; }

btSoftBodyWorldInfo* VRPhysicsManager::getSoftBodyWorldInfo() {return softBodyWorldInfo;}

void VRPhysicsManager::setPhysicsActive(bool a) {
    VRLock lock(*mtx);
    active = a;
    skip = true;
}

void VRPhysicsManager::prepareObjects() {
    for (auto o : OSGobjs) {
        if (auto so = o.second.lock()) {
            if (so->getPhysics()) so->getPhysics()->prepareStep();
        }
    }
}

void VRPhysicsManager::postprocessObjects() {
    for (auto o : OSGobjs) {
        if (auto so = o.second.lock()) {
            if (so->getPhysics()) so->getPhysics()->computeAccelerations();
        }
    }
}

void VRPhysicsManager::updatePhysics( VRThreadWeakPtr wthread) {
    VRTimer timer; timer.start();
    long long dt,t0,t1,t3;
    t1 = getTime();
    auto thread = wthread.lock();

    auto prof_id = VRProfiler::get()->regStart("physics simulation");
    if (active && thread && dynamicsWorld) {
        t0 = thread->t_last;
        thread->t_last = t1;
        dt = t1-t0;
        if (skip || dt < 0) { skip = 0; dt = 0; }

        {
            VRLock lock(*mtx);
            //double DT = 1.0/15000; // 1.0/500;
            double T = 1e-6*dt;

            prepareObjects();
            for (auto f : updateFktsPre) (*(f.lock()))();
            int n = dynamicsWorld->stepSimulation(T, 100, timestep);
            simulationTime += n*timestep;
            for (auto f : updateFktsPost) (*(f.lock()))();
            postprocessObjects();
        }
    }
    VRProfiler::get()->regStop(prof_id);

    prof_id = VRProfiler::get()->regStart("physics sleep");
    //t2 = getTime();
    //dt = t2-t1;
   // if (dt < 0) dt = 0;

    //sleep up to 500 fps
    //double pfT = PHYSICS_THREAD_TIMESTEP_MS * 1000;
    doFrameSleep(timer.stop(), 500);
    //if (dt < pfT) this_thread::sleep_for(chrono::microseconds(pfT -dt));
    t3 = getTime();

    if (active) {
        VRLock lock(*mtx);
        if (t3-t1 > 0) fps = 1e6/(t3-t1);
    }
    VRProfiler::get()->regStop(prof_id);
}

void VRPhysicsManager::setSimulationTimestep(double ts) { VRLock lock(*mtx); timestep = ts; }
double VRPhysicsManager::getSimulationTime() { VRLock lock(*mtx); return simulationTime; }

void VRPhysicsManager::addPhysicsUpdateFunction(VRUpdateCbPtr fkt, bool after) {
    VRLock lock(*mtx);
    if (after) updateFktsPost.push_back(fkt);
    else updateFktsPre.push_back(fkt);
}

void VRPhysicsManager::dropPhysicsUpdateFunction(VRUpdateCbPtr fkt, bool after) {
    VRLock lock(*mtx);
    auto& fkts = after ? updateFktsPost : updateFktsPre;
    for (unsigned int i = 0; i < fkts.size() ; i++) {
        if (fkts[i].lock() == fkt) { fkts.erase(fkts.begin() + i); return; }
    }
 }

void VRPhysicsManager::updateSpringsVisual() {
    if (!physics_visual_layer->getVisibility()) return;

    if (!springsVisual) {
        springsVisual = OSG::VRGeometry::create("phys_shape");
        springsVisual->addTag("SYSTEM:COLLISIONSHAPE");
        physics_visual_layer->addObject(springsVisual);

        auto m = VRMaterial::create("springsVisualMat");
        m->setLit(0);
        m->setLineWidth(2);
        m->setDiffuse(Color3f(0.3,0,0.6));
        m->setDepthTest(GL_ALWAYS);
        springsVisual->setMaterial(m);
    }

    VRGeoData data;
    //for (auto o : OSGobjs) if (auto so = o.second.lock())
    for (int i=0; i<dynamicsWorld->getNumConstraints(); i++) {
        btTypedConstraint* c = dynamicsWorld->getConstraint(i);
        btRigidBody& bA = c->getRigidBodyA();
        btRigidBody& bB = c->getRigidBodyB();

        Vec3d pA = VRPhysics::toVec3d( bA.getCenterOfMassPosition() );
        Vec3d pB = VRPhysics::toVec3d( bB.getCenterOfMassPosition() );

        btGeneric6DofSpringConstraint* s = dynamic_cast<btGeneric6DofSpringConstraint*>(c);
        if (s) {
            btTransform& fA = s->getFrameOffsetA();
            btTransform& fB = s->getFrameOffsetB();
            btTransform mA = bA.getCenterOfMassTransform();
            btTransform mB = bB.getCenterOfMassTransform();
            btTransform sA, sB;
            sA.mult(mA,fA);
            sB.mult(mB,fB);
            pA = VRPhysics::toVec3d( sA.getOrigin() );
            pB = VRPhysics::toVec3d( sB.getOrigin() );
        }

        data.pushVert(pA);
        data.pushVert(pB);
        data.pushLine();
    }
    data.apply(springsVisual);
}

void VRPhysicsManager::updatePhysObjects() {
    //VRTimer timer;
    //timer.start("D1");
    auto profiler = VRProfiler::get();

    int pID0 = profiler->regStart("wait phys lock");
    VRLock lock(*mtx);
    profiler->regStop(pID0);
    //timer.start("D2");
    VRGlobals::PHYSICS_FRAME_RATE.fps = fps;
    int pID1 = profiler->regStart("resolve physics objects");
    for (auto o : OSGobjs) if (auto so = o.second.lock()) so->resolvePhysics();
    profiler->regStop(pID1);

    updateSpringsVisual();

    //auto D = timer.stop("D1");
    //if (D > 3) cout << "      tt " << D << " " << timer.stop("D2") << endl;

    //the soft bodies
    btSoftBodyArray arr = dynamicsWorld->getSoftBodyArray();
    //Patches
    VRTransformPtr soft_trans;
    btSoftBody* patch;
    for(int i = 0; i < arr.size() ;i++) { //for all soft bodies
        soft_trans = OSGobjs[arr[i]].lock(); //get the corresponding transform to this soft body
        if (!soft_trans) continue;
        patch = arr[i]; //the soft body
        if (soft_trans->getType() == "Sprite") {
            OSG::VRGeometryPtr geo = static_pointer_cast<OSG::VRGeometry>(soft_trans);
            OSG::VRGeometryPtr visualgeo = physics_visuals[patch]; //render the visual

            btSoftBody::tNodeArray&   nodes(patch->m_nodes);
            btSoftBody::tFaceArray&   faces(patch->m_faces);
            btSoftBody::tLinkArray&   links(patch->m_links);
            GeoPnt3fPropertyMTRecPtr visualpos = GeoPnt3fProperty::create();
            GeoUInt32PropertyMTRecPtr visualinds = GeoUInt32Property::create();
            GeoVec3fPropertyMTRecPtr visualnorms = GeoVec3fProperty::create();

            for (int i = 0; i<nodes.size(); i++) { //go through the nodes and copy positions to mesh positionarray
                    Vec3d p = VRPhysics::toVec3d(nodes[i].m_x);
                    OSG::Vec3d tmp;
                    visualpos->addValue(p);
                    Vec3d n = VRPhysics::toVec3d(nodes[i].m_n);
                    visualnorms->addValue( n );
            }

            for(int j=0;j<faces.size();++j) {
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
            GeoUInt32PropertyMTRecPtr vtypes = GeoUInt32Property::create();
            GeoUInt32PropertyMTRecPtr vlens = GeoUInt32Property::create();
            vtypes->addValue(GL_TRIANGLES);
            vlens->addValue(faces.size());
            vtypes->addValue(GL_LINES);
            vlens->addValue(links.size());

            visualgeo->setType(GL_TRIANGLES    );
            visualgeo->setPositions(visualpos);
            visualgeo->setIndices(visualinds);
            visualgeo->setNormals(visualnorms);

            if(geo->getPrimitive()->getType() == "Plane") { //only for plane soft bodies : directly apply nodes to vertices of geometry model
                //VRPlane* prim = (VRPlane*)geo->getPrimitive();
                GeoPnt3fPropertyMTRecPtr positions = GeoPnt3fProperty::create();
                GeoVec3fPropertyMTRecPtr norms = GeoVec3fProperty::create();
                GeoUInt32PropertyMTRecPtr inds = GeoUInt32Property::create();
                for (int i = 0; i<nodes.size(); i++) { //go through the nodes and copy positions to mesh positionarray
                    Vec3d p = VRPhysics::toVec3d(nodes[i].m_x);
                    positions->addValue(p);
                    Vec3d n = VRPhysics::toVec3d(nodes[i].m_n);
                    norms->addValue( n );
                }
                geo->setPositions(positions);
                geo->setNormals(norms);
           }

        /*   if(soft_trans->getPhysics()->getShape() == "Rope") { //only for Ropes

            }*/

        }
    }
}

void VRPhysicsManager::physicalize(VRTransformPtr obj) {
    if (!obj) return;
    btCollisionObject* bdy = obj->getPhysics()->getCollisionObject();
    if (!bdy) return;
    OSGobjs[bdy] = obj;
}

void VRPhysicsManager::unphysicalize(VRTransformPtr obj) {
    if (!obj) return;
    btCollisionObject* bdy = obj->getPhysics()->getCollisionObject();
    if (!bdy) return;
    if (OSGobjs.count(bdy)) OSGobjs.erase(bdy);
}

void VRPhysicsManager::setGravity(Vec3d g) { VRLock lock(*mtx); dynamicsWorld->setGravity(btVector3(g[0],g[1],g[2])); }

btSoftRigidDynamicsWorld* VRPhysicsManager::bltWorld() { return dynamicsWorld; }

OSG_END_NAMESPACE;



