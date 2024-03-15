#include "VRPhysics.h"
#include "core/scene/VRScene.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRStroke.h"
#include "core/utils/VRVisualLayer.h"
#include "core/utils/VRTimer.h"
#include "core/utils/VRRate.h"
#include "core/math/kinematics/VRConstraint.h"
#include "core/math/pose.h"

#include <OpenSG/OSGTriangleIterator.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
//#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>

#ifndef WITHOUT_HACD
#include <HACD/hacdCircularList.h>
#include <HACD/hacdVector.h>
#include <HACD/hacdICHull.h>
#include <HACD/hacdGraph.h>
#include <HACD/hacdHACD.h>

#include <ConvexDecomposition/cd_wavefront.h>
#include <ConvexDecomposition/ConvexBuilder.h>
#endif
#include "core/utils/VRMutex.h"

using namespace OSG;



VRMutex& VRPhysics_mtx() {
    auto scene = OSG::VRScene::getCurrent();
    if (scene) return scene->physicsMutex();
    else {
        static VRMutex m;
        return m;
    };
}

struct VRPhysicsJoint {
    OSG::VRConstraintPtr constraint = 0;
    OSG::VRConstraintPtr spring = 0;
    VRPhysics* partner = 0;
    btGeneric6DofSpringConstraint* btJoint = 0;
    bool disableCollisions = true;

    VRPhysicsJoint() {;}

    ~VRPhysicsJoint() {
        if (btJoint) delete btJoint;
    }

    VRPhysicsJoint(VRPhysics* p, OSG::VRConstraintPtr c, OSG::VRConstraintPtr cs, bool dc) {
        constraint = c;
        spring = cs;
        partner = p;
        disableCollisions = dc;
        btJoint = 0;
    }
};

VRCollision::VRCollision() {}
VRCollision::~VRCollision() {}

Vec3d VRCollision::getPos1() { return pos1; }
Vec3d VRCollision::getPos2() { return pos2; }
Vec3d VRCollision::getNorm() { return norm; }
float VRCollision::getDistance() { return distance; }
VRTransformPtr VRCollision::getObj1() { return obj1.lock(); }
VRTransformPtr VRCollision::getObj2() { return obj2.lock(); }
vector<Vec4d> VRCollision::getTriangle1() { return triangle1; }
vector<Vec4d> VRCollision::getTriangle2() { return triangle2; }

VRPhysics::VRPhysics(OSG::VRTransformWeakPtr t) {
    vr_obj = t;
    activation_mode = ACTIVE_TAG;
}

VRPhysics::~VRPhysics() {
    VRLock lock(VRPhysics_mtx());
    clear();
}

btRigidBody* VRBtPhysics::getRigidBody() { VRLock lock(VRPhysics_mtx()); return body; }
btPairCachingGhostObject* VRBtPhysics::getGhostBody() { VRLock lock(VRPhysics_mtx()); return ghost_body; }
btCollisionShape* VRBtPhysics::getCollisionShape() { VRLock lock(VRPhysics_mtx()); return shape; }

OSG::Vec3d VRPhysics::toVec3d(btVector3 v) { return OSG::Vec3d(v[0], v[1], v[2]); }
btVector3 VRPhysics::toBtVector3(OSG::Vec3d v) { return btVector3(v[0], v[1], v[2]); }
OSG::Vec4d VRPhysics::toVec4d(btVector3 v) { return OSG::Vec4d(v[0], v[1], v[2], v.w()); }
btVector3 VRPhysics::toBtVector3(OSG::Vec4d v) { btVector3 b(v[0], v[1], v[2]); b.setW(v[3]); return b; }

void VRPhysics::setPhysicalized(bool b) { physicalized = b; update(); }
void VRPhysics::setShape(string s, float param) { physicsShape = s; shape_param = param; update(); }
bool VRPhysics::isPhysicalized() { return physicalized; }
string VRPhysics::getShape() { return physicsShape; }

bool VRPhysics::isDynamic() { return dynamic; }
void VRPhysics::setMass(float m) { mass = m; update(); }
float VRPhysics::getMass() { return mass; }
Vec3d VRPhysics::getCenterOfMass() { return CoMOffset; }
void VRPhysics::setFriction(float f) { friction = f; update(); }
float VRPhysics::getFriction() { return friction; }
void VRPhysics::setCollisionMargin(float m) { collisionMargin = m; update(); }
float VRPhysics::getCollisionMargin() { return collisionMargin; }
void VRPhysics::setCollisionGroup(int g) { collisionGroup = g; update(); }
void VRPhysics::setCollisionMask(int m) { collisionMask = m; update(); }
int VRPhysics::getCollisionGroup() { return collisionGroup; }
int VRPhysics::getCollisionMask() { return collisionMask; }
void VRPhysics::setActivationMode(int m) { activation_mode = m; update(); }
int VRPhysics::getActivationMode() { return activation_mode; }
void VRPhysics::setGhost(bool b) { bt.ghost = b; update(); }
bool VRPhysics::isGhost() { return bt.ghost; }
void VRPhysics::setSoft(bool b) { bt.soft = b; update(); }
bool VRPhysics::isSoft() { return bt.soft; }

void VRPhysics::setDamping(float lin, float ang, bool f) {
    linDamping = lin;
    angDamping = ang;
    if (!f) update();
    else {
        VRLock lock(VRPhysics_mtx());
        bt.body->setDamping(btScalar(linDamping), btScalar(angDamping));
    }
}

OSG::Vec3d VRPhysics::getForce() { VRLock lock(VRPhysics_mtx()); return toVec3d(bt.constantForce); }
OSG::Vec3d VRPhysics::getTorque() { VRLock lock(VRPhysics_mtx()); return toVec3d(bt.constantTorque); }

void VRPhysics::prepareStep() {
    if (bt.soft || !bt.body) return;
    auto f = bt.constantForce;
    auto t = bt.constantTorque;
    if (f.length2() > 0) bt.body->applyCentralForce(f);
    if (t.length2() > 0) bt.body->applyTorque(t);
}

btCollisionObject* VRBtPhysics::getCollisionObject() {
     VRLock lock(VRPhysics_mtx());
     if (ghost) return (btCollisionObject*)ghost_body;
     if (soft)  return (btCollisionObject*)soft_body;
     else return body;
}

vector<VRCollision> VRPhysics::getCollisions() {
    VRLock lock(VRPhysics_mtx());
    vector<VRCollision> res;
    if (!physicalized) return res;

    function<vector<Vec4d> (const btCollisionShape*, int, int, const btManifoldPoint&, btPersistentManifold*) > getShapeTriangle = [&](const btCollisionShape* shape, int partID, int triangleID, const btManifoldPoint& pt, btPersistentManifold* manifold) {
        int stype = shape->getShapeType();
        vector<Vec4d> res;
        if (stype == 8) return res; // sphere
        if (stype == 0) return res; // box
        if (stype == 24) return res; // heightmap
        if (stype == 4) return res; // convex, TODO: complete and validate

        if (stype == 21) { // trianglemesh
            auto tshpe = (btBvhTriangleMeshShape*)shape;
            auto tmsh = (btTriangleMesh*)tshpe->getMeshInterface();
            IndexedMeshArray& mesh = tmsh->getIndexedMeshArray();
            if (partID >= mesh.size()) return res;

            int Ni = mesh[partID].m_numTriangles;
            //int Nv = mesh[partID].m_numVertices;
            if (triangleID >= Ni) {
                cout << "VRPhysics::getCollisions, WARNING: triangleID " << triangleID << " to big! (" << Ni << ") N mesh: " << mesh.size() << endl;
                return res;
            }

            unsigned int* bt_inds = (unsigned int*)mesh[partID].m_triangleIndexBase;
            btVector3* verts = (btVector3*)mesh[partID].m_vertexBase;
            btVector3 vert1 = verts[bt_inds[triangleID*3+0]]; // first trianlge vertex
            btVector3 vert2 = verts[bt_inds[triangleID*3+1]]; // secon trianlge vertex
            btVector3 vert3 = verts[bt_inds[triangleID*3+2]]; // third trianlge vertex
            return vector<Vec4d>( { toVec4d(vert1), toVec4d(vert2), toVec4d(vert3) } );
        }

        if (stype == 31) { // compound
            return res;
            /*btCompoundShape* cpshape = (btCompoundShape*)shape;
            btCollisionShape* shape2 = cpshape->getChildShape( manifold->m_index1a ); // TODO: m_index1a does not work, no idea what to get here!
            return getShapeTriangle(shape2, triangleID, pt, manifold);*/
        }

        return res;
    };

    if (!bt.ghost) {
        int numManifolds = bt.world->getDispatcher()->getNumManifolds();
        for (int i=0;i<numManifolds;i++) {
            btPersistentManifold* manifold =  bt.world->getDispatcher()->getManifoldByIndexInternal(i);
            bool thisFirst = (manifold->getBody0() == bt.body);
            auto otherBody = thisFirst ? manifold->getBody1() : manifold->getBody0();
            auto otherObj = ((VRPhysics*)otherBody->getUserPointer())->vr_obj;

            int numContacts = manifold->getNumContacts();
            for (int j=0;j<numContacts;j++) {
                btManifoldPoint& pt = manifold->getContactPoint(j);
                if (pt.getDistance()<0.f) {
                    VRCollision c;
                    c.obj1 = vr_obj;
                    c.obj2 = otherObj;
                    c.pos1 = thisFirst ? toVec3d( pt.getPositionWorldOnA() ) : toVec3d( pt.getPositionWorldOnB() );
                    c.pos2 = thisFirst ? toVec3d( pt.getPositionWorldOnB() ) : toVec3d( pt.getPositionWorldOnA() );
                    c.norm = toVec3d( pt.m_normalWorldOnB );
                    c.distance = pt.getDistance();
                    auto t1 = getShapeTriangle( manifold->getBody0()->getCollisionShape(), pt.m_partId0, pt.m_index0, pt, manifold );
                    auto t2 = getShapeTriangle( manifold->getBody1()->getCollisionShape(), pt.m_partId1, pt.m_index1, pt, manifold );
                    c.triangleID1 = thisFirst ? pt.m_index0 : pt.m_index1;
                    c.triangleID2 = thisFirst ? pt.m_index1 : pt.m_index0;
                    c.triangle1 = thisFirst ? t1 : t2;
                    c.triangle2 = thisFirst ? t2 : t1;
                    res.push_back(c);
                }
            }
        }
        return res;
    }

    // --------- ghost object --------------
    btManifoldArray   manifoldArray;
    btBroadphasePairArray& pairArray = bt.ghost_body->getOverlappingPairCache()->getOverlappingPairArray();
    int numPairs = pairArray.size();

    for (int i=0;i<numPairs;i++) {
        manifoldArray.clear();

        const btBroadphasePair& pair = pairArray[i];

        //unless we manually perform collision detection on this pair, the contacts are in the dynamics world paircache:
        btBroadphasePair* collisionPair = bt.world->getPairCache()->findPair(pair.m_pProxy0,pair.m_pProxy1);
        if (!collisionPair) continue;
        if (collisionPair->m_algorithm) collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);

        for (int j=0;j<manifoldArray.size();j++) {
            btPersistentManifold* manifold = manifoldArray[j];
            auto otherBody = manifold->getBody0() == bt.ghost_body ? manifold->getBody1() : manifold->getBody0();
            auto otherObj = ((VRPhysics*)otherBody->getUserPointer())->vr_obj;

            btScalar directionSign = manifold->getBody0() == bt.ghost_body ? btScalar(-1.0) : btScalar(1.0);
            for (int p=0;p<manifold->getNumContacts();p++) {
                const btManifoldPoint& pt = manifold->getContactPoint(p);
                if (pt.getDistance()<0.f) {
                    VRCollision c;
                    c.obj1 = vr_obj;
                    c.obj2 = otherObj;
                    c.pos1 = toVec3d( pt.getPositionWorldOnA() );
                    c.pos2 = toVec3d( pt.getPositionWorldOnB() );
                    c.norm = toVec3d( pt.m_normalWorldOnB*directionSign );
                    c.distance = pt.getDistance();
                    res.push_back(c);
                }
            }
        }
    }


    return res;
}

vector<string> VRPhysics::getPhysicsShapes() {
    static vector<string> shapes;
    if (shapes.size() == 0) {
        shapes.push_back("Box");
        shapes.push_back("Sphere");
        shapes.push_back("Convex");
        shapes.push_back("Concave");
        shapes.push_back("ConvexDecomposed");
        shapes.push_back("Cloth");
        shapes.push_back("Rope");
    }
    return shapes;
}

void VRPhysics::setCenterOfMass(OSG::Vec3d com) {
    CoMOffset = com;
    comType = "custom";
    update();
}

void VRPhysics::clear() {
    auto scene = OSG::VRScene::getCurrent();
    if (scene) scene->unphysicalize(vr_obj.lock());

    if (scene) bt.world = scene->bltWorld();
    else bt.world = 0;

    if (bt.body != 0) {
        for (auto j : joints) {
            VRPhysics* partner = j.first;
            VRPhysicsJoint* joint = j.second;

            if (joint && joint->btJoint != 0) {
                if (bt.world) bt.world->removeConstraint(joint->btJoint);
                delete joint->btJoint;
                joint->btJoint = 0;
            }

            if (partner && partner->joints2.count(this)) partner->joints2.erase(this);
        }
        joints.clear();

        for (auto j : joints2) {
            VRPhysics* partner = j.first;
            VRPhysicsJoint* joint = j.second;

            if (joint && joint->btJoint != 0) {
                if (bt.world) bt.world->removeConstraint(joint->btJoint);
                delete joint->btJoint;
                joint->btJoint = 0;
            }

            if (partner && partner->joints.count(this)) partner->joints.erase(this);
        }
        joints2.clear();

        if (bt.world) bt.world->removeRigidBody(bt.body);
        delete bt.body;
        bt.body = 0;
    }

    if (bt.ghost_body != 0) {
        if (bt.world) bt.world->removeCollisionObject(bt.ghost_body);
        delete bt.ghost_body;
        bt.ghost_body = 0;
    }

    if (bt.soft_body != 0) {
        if (bt.world) bt.world->removeCollisionObject(bt.soft_body);
        delete bt.soft_body;
        bt.soft_body = 0;
    }

    if (bt.shape != 0 && bt.shape != bt.customShape) delete bt.shape;
    if (bt.shape != 0) bt.shape = 0;
    if (bt.motionState != 0) { delete bt.motionState; bt.motionState = 0; }

    if (visShape) visShape->destroy();
    visShape.reset();
}

void VRPhysics::triggerCallbacks(btManifoldPoint* cp, const btCollisionObjectWrapper* obj1, const btCollisionObjectWrapper* obj2 ) {
    btCollision c;
    c.cp = cp;
    c.obj1 = (btCollisionObjectWrapper*)obj1;
    c.obj2 = (btCollisionObjectWrapper*)obj2;
    if (callback) (*callback)(c);
}

bool customContactAddedCallback( btManifoldPoint& cp, const btCollisionObjectWrapper* obj1, int partID1, int ID1, const btCollisionObjectWrapper* obj2, int partID2, int ID2) {
    bool isCBObj1 = obj1->getCollisionObject()->getCollisionFlags() & btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK;
    bool isCBObj2 = obj2->getCollisionObject()->getCollisionFlags() & btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK;
    VRPhysics* phys = 0;
    if (isCBObj1) phys = (VRPhysics*)obj1->getCollisionObject()->getUserPointer();
    if (isCBObj2) phys = (VRPhysics*)obj2->getCollisionObject()->getUserPointer();
    if (phys) phys->triggerCallbacks(&cp, obj1, obj2);
	return true; // ignored
}

void VRPhysics::setDynamic(bool b, bool fast) {
    dynamic = b;
    if (fast && bt.body) {
        VRLock lock(VRPhysics_mtx());
        if (!b) {
            bt.body->setMassProps(0, btVector3(0,0,0));
            bt.body->setCollisionFlags(bt.body->getCollisionFlags() |  btCollisionObject::CF_STATIC_OBJECT);
        } else {
            updateTransformation(vr_obj.lock());
            bt.body->setMassProps(mass, bt.inertia);
            bt.body->setCollisionFlags(bt.body->getCollisionFlags() & ~btCollisionObject::CF_STATIC_OBJECT);
        }
    } else { update(); }
}

void VRPhysics::setGravity(OSG::Vec3d v) {
    bool fast = true;
    bt.gravity = toBtVector3(v);
    if (fast && bt.body) {
        VRLock lock(VRPhysics_mtx());
        bt.body->setGravity(bt.gravity);
    } else { update(); }
}

void VRPhysics::update() { // only called when object is physicalized
    auto scene = OSG::VRScene::getCurrent();
    if (scene == 0) return;

    if (bt.world == 0) bt.world = scene->bltWorld();
    if (bt.world == 0) return;

    VRLock lock(VRPhysics_mtx());
    clear();

    if (!physicalized) return;

    float _mass = mass;
    if (!dynamic || paused) _mass = 0;

    if (bt.soft) {
        if (physicsShape == "Cloth") bt.soft_body = createCloth();
        if (physicsShape == "Rope") bt.soft_body = createRope();
        if (bt.soft_body == 0) { return; }
        bt.soft_body->setActivationState(activation_mode);
        bt.world->addSoftBody(bt.soft_body,collisionGroup, collisionMask);
        scene->physicalize(vr_obj.lock());
        updateConstraints();
        return;
    }

    CoMOffset = OSG::Vec3d(0,0,0);
    if (comType == "custom") CoMOffset = CoMOffset_custom;
    if (physicsShape == "Custom") bt.shape = bt.customShape;
    if (physicsShape == "Compound") bt.shape = getCompoundShape();
    if (physicsShape == "Box") bt.shape = getBoxShape();
    if (physicsShape == "Sphere") bt.shape = getSphereShape();
    if (physicsShape == "Convex") bt.shape = getConvexShape(CoMOffset);
    if (physicsShape == "Concave") bt.shape = getConcaveShape();
    if (physicsShape == "ConvexDecomposed") bt.shape = getHACDShape();
    if (useCallbacks) gContactAddedCallback = customContactAddedCallback;
    if (bt.shape == 0) { cout << "ERROR: physics shape unknown: " << physicsShape << endl; return; }

    bool scaleChanged = false;
    bt.motionState = new btDefaultMotionState( fromVRTransform( vr_obj.lock(), scale, CoMOffset, scaleChanged ) );
    if (scaleChanged) rescaleCollisionShape();

    if (_mass != 0) bt.shape->calculateLocalInertia(_mass, bt.inertia);

    if (bt.ghost) {
        bt.ghost_body = new btPairCachingGhostObject();
        bt.ghost_body->setCollisionShape( bt.shape );
        if ( auto sp = vr_obj.lock() ) bt.ghost_body->setUserPointer( sp.get() );
        bt.ghost_body->setCollisionFlags( btCollisionObject::CF_KINEMATIC_OBJECT | btCollisionObject::CF_NO_CONTACT_RESPONSE );
        bt.world->addCollisionObject(bt.ghost_body, collisionGroup, collisionMask);
        bt.ghost_body->setUserPointer(this);
    } else {
        btRigidBody::btRigidBodyConstructionInfo rbInfo( _mass, bt.motionState, bt.shape, bt.inertia );
        bt.body = new btRigidBody(rbInfo);
        if (useCallbacks) bt.body->setCollisionFlags(bt.body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
        bt.body->setActivationState(activation_mode);
        bt.body->setDamping(btScalar(linDamping), btScalar(angDamping));
        bt.world->addRigidBody(bt.body, collisionGroup, collisionMask);
        bt.body->setGravity(bt.gravity);
        bt.body->setFriction(friction);
        bt.body->setUserPointer(this);
    }

    scene->physicalize(vr_obj.lock());
    updateConstraints();

    if (!visShape) createVisualGeo();

    //if (visShape->isVisible())
    updateVisualGeo(); // TODO: only when the visuallayer toggles, maybe a callback somewhere??
}

void VRPhysics::setCollisionCallback(CallbackPtr cb) { callback = cb; useCallbacks = true; update(); }

btSoftBody* VRPhysics::createCloth() {
    auto obj = vr_obj.lock();
    if (!obj) return 0;

    if ( !obj->hasTag("geometry") ) { cout << "VRPhysics::createCloth only works on geometries" << endl; return 0; }
    OSG::VRGeometryPtr geo = static_pointer_cast<OSG::VRGeometry>(obj);
    if ( geo->getPrimitive()->getType() != "Plane") { cout << "VRPhysics::createCloth only works on Plane primitives" << endl; return 0; }

    OSG::Matrix4d m = geo->getMatrix();//get Transformation
    geo->setIdentity();
    btSoftBodyWorldInfo* info = OSG::VRScene::getCurrent()->getSoftBodyWorldInfo();

    VRPlane* prim = (VRPlane*)geo->getPrimitive();
    float nx = prim->Nx;
    float ny = prim->Ny;
    //float h = prim->height;
    //float w = prim->width;

    OSG::GeoVectorPropertyMTRecPtr positions = geo->getMesh()->geo->getPositions();
    vector<btVector3> vertices;
    vector<btScalar> masses;

    OSG::Pnt3d p;
    for(uint i = 0; i < positions->size();i++) { //add all vertices
        positions->getValue(p,i);
        m.mult(p,p);
        vertices.push_back( toBtVector3(OSG::Vec3d(p)) );
        masses.push_back(5.0);
    }

    btVector3* start = &vertices.front();
    btScalar* startm = &masses.front();
    btSoftBody* ret = new btSoftBody(info,(int)positions->size(),start,startm);


    //nx is segments in polyvr, but we need resolution ( #vertices in x direction) so nx+1 and <= nx
    #define IDX(_x_,_y_)    ((_y_)*(nx+1)+(_x_))
    /* Create links and faces */
    for(int iy=0;iy<=ny;++iy) {
        for(int ix=0;ix<=nx;++ix) {
            const int       idx=IDX(ix,iy);
            const bool      mdx=(ix+1)<=nx;
            const bool      mdy=(iy+1)<=ny;
            if(mdx) ret->appendLink(idx,IDX(ix+1,iy));
            if(mdy) ret->appendLink(idx,IDX(ix,iy+1));
            if(mdx&&mdy) {
                if((ix+iy)&1) {
                    ret->appendFace(IDX(ix,iy),IDX(ix+1,iy),IDX(ix+1,iy+1));
                    ret->appendFace(IDX(ix,iy),IDX(ix+1,iy+1),IDX(ix,iy+1));
                    ret->appendLink(IDX(ix,iy),IDX(ix+1,iy+1));
                } else {
                    ret->appendFace(IDX(ix,iy+1),IDX(ix,iy),IDX(ix+1,iy));
                    ret->appendFace(IDX(ix,iy+1),IDX(ix+1,iy),IDX(ix+1,iy+1));
                    ret->appendLink(IDX(ix+1,iy),IDX(ix,iy+1));
                }
            }
        }
    }
    #undef IDX

    return ret;
}

btSoftBody* VRPhysics::createRope() { // TODO
    auto obj = vr_obj.lock();
    if (!obj) return 0;

    OSG::VRStrokePtr geo = dynamic_pointer_cast<OSG::VRStroke>(obj);
    if ( !geo ) { cout << "VRPhysics::createCloth only works on stroke geometries" << endl; return 0; }

    OSG::Matrix4d m = geo->getMatrix();//get Transformation
    geo->setIdentity();
    btSoftBodyWorldInfo* info = OSG::VRScene::getCurrent()->getSoftBodyWorldInfo();

    auto profile = geo->getProfile();
    auto path = geo->getPath();

    OSG::GeoVectorPropertyMTRecPtr positions = geo->getMesh()->geo->getPositions();
    vector<btVector3> vertices;
    vector<btScalar> masses;

    OSG::Pnt3d p;
    for (uint i = 0; i < positions->size();i++) { //add all vertices
        positions->getValue(p,i);
        m.mult(p,p);
        vertices.push_back( toBtVector3(OSG::Vec3d(p)) );
        masses.push_back(5.0);
    }

    btSoftBody* ret = new btSoftBody(info, (int)positions->size(), &vertices[0], &masses[0]);

    /*#define IDX(_x_,_y_)    ((_y_)*(nx+1)+(_x_))
    for(int iy=0;iy<=ny;++iy) {
        for(int ix=0;ix<=nx;++ix) {
            const int       idx=IDX(ix,iy);
            const bool      mdx=(ix+1)<=nx;
            const bool      mdy=(iy+1)<=ny;
            if(mdx) ret->appendLink(idx,IDX(ix+1,iy));
            if(mdy) ret->appendLink(idx,IDX(ix,iy+1));
            if(mdx&&mdy) {
                if((ix+iy)&1) {
                    ret->appendFace(IDX(ix,iy),IDX(ix+1,iy),IDX(ix+1,iy+1));
                    ret->appendFace(IDX(ix,iy),IDX(ix+1,iy+1),IDX(ix,iy+1));
                    ret->appendLink(IDX(ix,iy),IDX(ix+1,iy+1));
                } else {
                    ret->appendFace(IDX(ix,iy+1),IDX(ix,iy),IDX(ix+1,iy));
                    ret->appendFace(IDX(ix,iy+1),IDX(ix+1,iy),IDX(ix+1,iy+1));
                    ret->appendLink(IDX(ix+1,iy),IDX(ix,iy+1));
                }
            }
        }
    }
    #undef IDX*/

    return ret;
}

void VRPhysics::physicalizeTree(bool b) { physTree = b; cout << "VRPhysics::physicalizeTree " << physTree << endl; update(); }

vector<OSG::VRGeometryPtr> VRPhysics::getGeometries() {
    vector<OSG::VRGeometryPtr> res;
    auto obj = vr_obj.lock();
    if (!obj) return res;

    if (physTree) {
        for (auto o : obj->getChildren(true, "", true)) {
            OSG::VRGeometryPtr geo = dynamic_pointer_cast<OSG::VRGeometry>( o );
            if (geo) res.push_back( geo );
        }
    } else {
        OSG::VRGeometryPtr geo = dynamic_pointer_cast<OSG::VRGeometry>( obj );
        if (geo) res.push_back(geo);
    }

    return res;
}

OSG::GeometryMTRecPtr getMeshGeometry(VRGeometryPtr geo) {
    if (!geo) return 0;
    if (!geo->getMesh()) return 0;
    return geo->getMesh()->geo;
}

OSG::GeoVectorPropertyMTRecPtr getMeshPositions(VRGeometryPtr geo) {
    if (!geo) return 0;
    if (!geo->getMesh()) return 0;
    if (!geo->getMesh()->geo) return 0;
    return geo->getMesh()->geo->getPositions();
}

btCollisionShape* VRPhysics::getBoxShape() {
    auto obj = vr_obj.lock();
    if (!obj) return 0;

    if (shape_param > 0) return new btBoxShape( btVector3(shape_param, shape_param, shape_param) );
    double x,y,z;
    x=y=z=0;

    for (auto geo : getGeometries()) {
        auto pos = getMeshPositions(geo);
        if (!pos) continue;
		for (unsigned int i = 0; i<pos->size(); i++) {
            OSG::Vec3d p;
            pos->getValue(p,i);
            x = max(x, p[0]*scale[0]);
            y = max(y, p[1]*scale[1]);
            z = max(z, p[2]*scale[2]);
        }
    }

    //cout << "create BoxShape " << x << " " << y << " " << z << " " << scale << endl;
    return new btBoxShape(btVector3(x,y,z));
}

btCollisionShape* VRPhysics::getSphereShape() {
    auto obj = vr_obj.lock();
    if (!obj) return 0;

    if (shape_param > 0) return new btSphereShape( shape_param );

    double r2 = 0;
    //int N = 0;
    OSG::Vec3d p;
    OSG::Vec3d center;

    /*for (auto _g : geos ) { // get geometric center // makes no sense as you would have to change the center of the shape..
        OSG::VRGeometryPtr geo = (OSG::VRGeometryPtr)_g;
        OSG::GeoVectorPropertyMTRecPtr pos = geo->getMesh()->geo->getPositions();
        N += pos->size();
        for (unsigned int i=0; i<pos->size(); i++) {
            pos->getValue(p,i);
            center += p;
        }
    }

    center *= 1.0/N;*/

    for (auto geo : getGeometries()) {
        auto pos = getMeshPositions(geo);
        if (!pos) continue;
		for (unsigned int i = 0; i<pos->size(); i++) {
            pos->getValue(p,i);
            //r2 = max( r2, (p-center).squareLength() );
            for (int i=0; i<3; i++) p[i] *= scale[i];
            r2 = max( r2, p[0]*p[0]+p[1]*p[1]+p[2]*p[2] );
        }
    }

    return new btSphereShape(sqrt(r2));
}

btCollisionShape* VRPhysics::getConvexShape(OSG::Vec3d& mc) {
    auto obj = vr_obj.lock();
    if (!obj) return 0;

    OSG::Matrix4d m;
    OSG::Matrix4d M = obj->getWorldMatrix();
    M.invert();
    vector<OSG::Vec3d> points;

    if (comType == "geometric") mc = OSG::Vec3d(); // center of mass
    for (auto geo : getGeometries()) {
        auto pos = getMeshPositions(geo);
        if (!pos) continue;

        if (geo != obj) {
            m = geo->getWorldMatrix();
            m.multLeft(M);
        }

		for (unsigned int i = 0; i<pos->size(); i++) {
            OSG::Vec3d p;
            pos->getValue(p,i);
            if (geo != obj) m.mult(p,p);
            for (int i=0; i<3; i++) p[i] *= scale[i];
            points.push_back(p);
            if (comType == "geometric") mc += OSG::Vec3d(p);
        }
    }

    if (comType == "geometric") mc *= 1.0/points.size(); // displace around center of mass (approx. geom center)

    btConvexHullShape* shape = new btConvexHullShape();
    for (OSG::Vec3d& p : points) shape->addPoint(btVector3(p[0]-mc[0], p[1]-mc[1], p[2]-mc[2]));
    shape->setMargin(collisionMargin);
    return shape;
}

btCollisionShape* VRPhysics::getConcaveShape() {
    auto obj = vr_obj.lock();
    if (!obj) return 0;

    btTriangleMesh* tri_mesh = new btTriangleMesh();

    int N = 0;
    for (auto geo : getGeometries()) {
        auto g = getMeshGeometry(geo);
        if (!g) continue;
        OSG::TriangleIterator ti(g);

        btVector3 vertexPos[3];

        while(!ti.isAtEnd()) {
            for (int i=0;i<3;i++) {
                OSG::Pnt3f p = ti.getPosition(i);
                for (int j=0;j<3;j++) vertexPos[i][j] = p[j];
            }

            tri_mesh->addTriangle(vertexPos[0]*scale[0], vertexPos[1]*scale[1], vertexPos[2]*scale[2]);
            ++ti;
            N++;
        }
    }
    if (N == 0) return 0;

    //cout << "\nConstruct Concave shape for " << vr_obj->getName() << endl;
    btBvhTriangleMeshShape* shape = new btBvhTriangleMeshShape(tri_mesh, true);
    return shape;
}

btCollisionShape* VRPhysics::getCompoundShape() {
    auto obj = vr_obj.lock();
    if (!obj) return 0;

    btCompoundShape* shape = new btCompoundShape();

    OSG::Matrix4d m;
    OSG::Matrix4d M = obj->getWorldMatrix();
    M.invert();

    for (auto geo : getGeometries()) {
        auto pos = getMeshPositions(geo);
        if (!pos) continue;

        if (geo != obj) {
            m = geo->getWorldMatrix();
            m.multLeft(M);
        }

        vector<OSG::Vec3d> points;
		for (unsigned int i = 0; i<pos->size(); i++) {
            OSG::Vec3d p;
            pos->getValue(p,i);
            if (geo != obj) m.mult(p,p);
            for (int i=0; i<3; i++) p[i] *= scale[i];
            points.push_back(p);
        }

        btConvexHullShape* child = new btConvexHullShape();
        for (OSG::Vec3d& p : points) child->addPoint(btVector3(p[0], p[1], p[2]));

        child->setMargin(collisionMargin);
        btTransform T;
        T.setIdentity();
        shape->addChildShape(T, child);
    }

    return shape;
}

#ifndef WITHOUT_HACD
class MyConvexDecomposition : public ConvexDecomposition::ConvexDecompInterface {
    public:
        btAlignedObjectArray<btConvexHullShape*> m_convexShapes;
        int mBaseCount = 0;
        int mHullCount = 0;
        btVector3 centroid = btVector3(0,0,0);

    public:
        MyConvexDecomposition() {}

        virtual void ConvexDecompResult(ConvexDecomposition::ConvexResult &result) {
            btAlignedObjectArray<btVector3> vertices;

            for (unsigned int i=0; i<result.mHullVcount; i++) {
                btVector3 vertex(result.mHullVertices[i*3], result.mHullVertices[i*3+1], result.mHullVertices[i*3+2]);
                vertices.push_back(vertex);
            }

            btConvexHullShape* convexShape = new btConvexHullShape(&(vertices[0].getX()),vertices.size());
            m_convexShapes.push_back(convexShape);
        }
};
#endif

void VRPhysics::setConvexDecompositionParameters(float cw, float vw, float nc, float nv, float c, bool aedp, bool andp, bool afp) {
    compacityWeight = cw;
    volumeWeight = vw;
    NClusters = nc;
    NVerticesPerCH = nv;
    concavity = c;
    addExtraDistPoints = aedp;
    addNeighboursDistPoints = andp;
    addFacesPoints = afp;
}

btCollisionShape* VRPhysics::getHACDShape() {
#ifndef WITHOUT_HACD
    OSG::VRGeometryPtr obj = dynamic_pointer_cast<OSG::VRGeometry>( vr_obj.lock() );
    if (!obj) { cout << "Warning in getHACDShape: not a geometry!"; return 0; }

    std::vector< HACD::Vec3<HACD::Real> > points;
    std::vector< HACD::Vec3<long> > triangles;

    OSG::GeoVectorPropertyMTRecPtr positions = obj->getMesh()->geo->getPositions();
    OSG::Pnt3d p;
    for(uint i = 0; i < positions->size();i++) {
        positions->getValue(p,i);
        HACD::Vec3<HACD::Real> vertex(p[0], p[1], p[2]);
        points.push_back(vertex);
    }

    for (OSG::TriangleIterator it = OSG::TriangleIterator(obj->getMesh()->geo); !it.isAtEnd() ;++it) {
        HACD::Vec3<long> triangle(it.getPositionIndex(0), it.getPositionIndex(1), it.getPositionIndex(2));
        triangles.push_back(triangle);
    }

    HACD::HACD myHACD;
    myHACD.SetPoints(&points[0]);
    myHACD.SetNPoints(points.size());
    myHACD.SetTriangles(&triangles[0]);
    myHACD.SetNTriangles(triangles.size());
    myHACD.SetCompacityWeight(compacityWeight);
    myHACD.SetVolumeWeight(volumeWeight);
    myHACD.SetNClusters(NClusters);                       // minimum number of clusters
    myHACD.SetNVerticesPerCH(NVerticesPerCH);                // max of 100 vertices per convex-hull
    myHACD.SetConcavity(concavity);                     // maximum concavity
    myHACD.SetAddExtraDistPoints(addExtraDistPoints);
    myHACD.SetAddNeighboursDistPoints(addNeighboursDistPoints);
    myHACD.SetAddFacesPoints(addFacesPoints);

    // Idee: split geometries into chunks? -> holes may be a problem!
    // pack this into thread
    // startThread( hacd compute )
    // startJob ( set Shape )
    VRTimer timer;
    timer.start("t");
    myHACD.Compute();
    timer.stop("t");
    timer.print();

    MyConvexDecomposition convexDecomposition;
    btCompoundShape* shape = new btCompoundShape();

    for (uint c=0; c < myHACD.GetNClusters(); c++) {
        size_t nPoints = myHACD.GetNPointsCH(c);
        size_t nTriangles = myHACD.GetNTrianglesCH(c);

        float* vertices = new float[nPoints*3];
        unsigned int* triangles = new unsigned int[nTriangles*3];

        HACD::Vec3<HACD::Real> * pointsCH = new HACD::Vec3<HACD::Real>[nPoints];
        HACD::Vec3<long> * trianglesCH = new HACD::Vec3<long>[nTriangles];
        myHACD.GetCH(c, pointsCH, trianglesCH);

        for(size_t v = 0; v < nPoints; v++) { // points
            vertices[3*v] = pointsCH[v].X();
            vertices[3*v+1] = pointsCH[v].Y();
            vertices[3*v+2] = pointsCH[v].Z();
        }

        for(size_t f = 0; f < nTriangles; f++) { // triangles
            triangles[3*f] = trianglesCH[f].X();
            triangles[3*f+1] = trianglesCH[f].Y();
            triangles[3*f+2] = trianglesCH[f].Z();
        }

        delete [] pointsCH;
        delete [] trianglesCH;

        ConvexDecomposition::ConvexResult r(nPoints, vertices, nTriangles, triangles);
        convexDecomposition.ConvexDecompResult(r);
    }

    for (int i=0; i < convexDecomposition.m_convexShapes.size(); i++) {
        btTransform trans;
        trans.setIdentity();
        auto cs = convexDecomposition.m_convexShapes[i];
        cs->setMargin(collisionMargin);
        shape->addChildShape(trans, cs );
    }

    return shape;
#else
    return 0;
#endif
}

void VRPhysics::setCustomShape(btCollisionShape* shape) { bt.customShape = bt.shape; physicsShape = "Custom"; }

void VRPhysics::createVisualGeo() {
    if (visShape) return;
    visShape = OSG::VRGeometry::create("phys_shape");
    visShape->addTag("SYSTEM:COLLISIONSHAPE");
    auto scene = OSG::VRScene::getCurrent();
    scene->getVisualLayer()->addObject(visShape);
}

OSG::VRTransformPtr VRPhysics::getVisualShape() {
    if (!visShape) createVisualGeo();
    updateVisualGeo();
    return visShape;
}

class heightData : public btTriangleCallback {
    public:
        ~heightData() {};

        void processTriangle(btVector3* triangle, int partId, int triangleIndex) {
            //cout << " --- processTriangle v1 " << VRPhysics::toVec3d(triangle[0]) << " v2 " << VRPhysics::toVec3d(triangle[1]) << " v3 " << VRPhysics::toVec3d(triangle[2]) << endl;
            data.pushVert(VRPhysics::toVec3d(triangle[0]));
            data.pushVert(VRPhysics::toVec3d(triangle[1]));
            data.pushVert(VRPhysics::toVec3d(triangle[2]));
            data.pushTri();
        }

        OSG::VRGeoData data;
};

void VRPhysics::updateVisualGeo() {
    auto geo = visShape;
    if (!geo) return;
    btCollisionShape* shape = bt.getCollisionShape();
    if (!shape) return;
    int stype = shape->getShapeType();

    // 0 : box
    // 4 : convex
    // 8 : sphere
    // 21 : trianglemesh
    // 24 : heightmap
    // 31 : compound

    if (stype == 8) { // sphere
        btSphereShape* sshape = (btSphereShape*)shape;
        btScalar radius = sshape->getRadius();
        stringstream params;
        params << radius*1.01 << " 2";
        geo->setPrimitive("Sphere " + params.str());
    }

    if (stype == 0) { // box
        btBoxShape* bshape = (btBoxShape*)shape;
        btVector4 plane;
        OSG::Vec3d dim;
        bshape->getPlaneEquation(plane, 0); dim[0] = 2*(abs(plane[3]) + shape->getMargin());
        bshape->getPlaneEquation(plane, 2); dim[1] = 2*(abs(plane[3]) + shape->getMargin());
        bshape->getPlaneEquation(plane, 4); dim[2] = 2*(abs(plane[3]) + shape->getMargin());
        stringstream params;
        params << dim[0]*1.01 << " " << dim[1]*1.01 << " " << dim[2]*1.01 << " 1 1 1";
        geo->setPrimitive("Box " + params.str());
    }

    if (stype == 4) { // convex
        btConvexHullShape* cshape = (btConvexHullShape*)shape;
        btShapeHull hull(cshape);
        hull.buildHull(cshape->getMargin());

        int Ni = hull.numIndices();
        int Nv = hull.numVertices();
        const unsigned int* bt_inds = hull.getIndexPointer();
        const btVector3* verts = hull.getVertexPointer();

        OSG::VRGeoData data;
        for (int i=0; i<Nv; i++) {
            OSG::Vec3d p = VRPhysics::toVec3d(verts[i]);
            OSG::Vec3d n = p; n.normalize();
            p += CoMOffset;
            data.pushVert(p,n);
        }

        for (int i=0; i<Ni; i+=3) {
            int i0 = bt_inds[i];
            int i1 = bt_inds[i+1];
            int i2 = bt_inds[i+2];
            data.pushTri(i0,i1,i2);
        }
        if (data.size()) data.apply(geo);
    }

    if (stype == 21) { // trianglemesh
        auto tshpe = (btBvhTriangleMeshShape*)shape;
        auto tmsh = (btTriangleMesh*)tshpe->getMeshInterface();
        IndexedMeshArray& meshes = tmsh->getIndexedMeshArray();
        OSG::VRGeoData data;

        for (int i=0; i<meshes.size(); i++) {
            auto& mesh = meshes[i];
            int Ni0 = data.size();

            int Ni = mesh.m_numTriangles;
            int Nv = mesh.m_numVertices;
            unsigned int* bt_inds = (unsigned int*)mesh.m_triangleIndexBase;
            btVector3* verts = (btVector3*)mesh.m_vertexBase;

            for (int i=0; i<Nv; i++) {
                OSG::Vec3d p = VRPhysics::toVec3d(verts[i]);
                OSG::Vec3d n = p; n.normalize();
                p += CoMOffset;
                data.pushVert(p,n);
            }

            for (int i=0; i<Ni; i++) {
                int i0 = Ni0 + bt_inds[i*3];
                int i1 = Ni0 + bt_inds[i*3+1];
                int i2 = Ni0 + bt_inds[i*3+2];
                data.pushTri(i0,i1,i2);
            }
        }

        if (data.size()) data.apply(geo);
    }

    if (stype == 24) { // heightmap, TODO, it works, but takes up way too much memory!
        /*auto hshpe = dynamic_cast<btConcaveShape*>(shape);
        if (!hshpe) return;
        float Max = 1e36;
        btVector3 aabbMin(-Max, -Max, -Max);
        btVector3 aabbMax(Max, Max, Max);

        heightData hdata;
        auto scene = OSG::VRScene::getCurrent();
        hshpe->processAllTriangles(&hdata, aabbMin, aabbMax);
        hdata.data.apply(geo);*/
    }

    if (stype == 31) { // compound
        btCompoundShape* cpshape = (btCompoundShape*)shape;
        OSG::VRGeoData data;

        for (int j = 0; j < cpshape->getNumChildShapes(); j++) {
            btCollisionShape* shape2 = cpshape->getChildShape(j);
            int s2type = shape2->getShapeType();

            int Ni = 0;
            int Nv = 0;
            unsigned int* bt_inds = 0;
            btVector3* verts = 0;

            if (s2type == 21) {
                auto tshpe = (btBvhTriangleMeshShape*)shape2;
                auto tmsh = (btTriangleMesh*)tshpe->getMeshInterface();
                IndexedMeshArray& mesh = tmsh->getIndexedMeshArray();
                if (mesh.size() == 0) return;

                Ni = mesh[0].m_numTriangles;
                Nv = mesh[0].m_numVertices;
                bt_inds = (unsigned int*)mesh[0].m_triangleIndexBase;
                verts = (btVector3*)mesh[0].m_vertexBase;
            }

            if (s2type == 4) {
                btConvexHullShape* cshape = (btConvexHullShape*)shape2;
                btShapeHull hull(cshape);
                hull.buildHull(cshape->getMargin());

                Ni = hull.numIndices();
                Nv = hull.numVertices();
                bt_inds = (unsigned int*)hull.getIndexPointer();
                verts = (btVector3*)hull.getVertexPointer();
            }


            int I0 = data.size();
            for (int i=0; i<Nv; i++) {
                OSG::Vec3d p = VRPhysics::toVec3d(verts[i]);
                OSG::Vec3d n = p; n.normalize();
                p += CoMOffset;
                data.pushVert(p,n);
            }

            for (int i=0; i<Ni; i++) {
                int i0 = bt_inds[i*3];
                int i1 = bt_inds[i*3+1];
                int i2 = bt_inds[i*3+2];
                if (i0 < 0 || i0 >= Nv || i1 < 0 || i1 >= Nv || i2 < 0 || i2 >= Nv) continue;
                data.pushTri(I0+i0,I0+i1,I0+i2);
            }
        }

        if (data.size()) data.apply(geo, true, true);
    }

    auto mat = OSG::VRMaterial::get("phys_mat");
    mat->setZOffset(-2,-2);
    geo->setMaterial(mat);
    geo->setWorldMatrix( getTransformation() );
}

void VRPhysics::rescaleCollisionShape() {
    Vec3d rescale = Vec3d(scale[0]/shapeScale[0], scale[1]/shapeScale[1], scale[2]/shapeScale[2]);
    shapeScale = scale;

    btCollisionShape* shape = bt.getCollisionShape();
    if (!shape) return;
    int stype = shape->getShapeType();

    if (stype == 8) { // sphere
        btSphereShape* sshape = (btSphereShape*)shape;
        sshape->setLocalScaling(toBtVector3(scale));
        //btScalar r = sshape->getRadius();
        //sshape->setRadius(r*rescale[0]);
    }

    if (stype == 0) { // box
        btBoxShape* bshape = (btBoxShape*)shape;
        bshape->setLocalScaling(toBtVector3(scale));
    }

    if (stype == 4) { // convex
        btConvexHullShape* cshape = (btConvexHullShape*)shape;
        cshape->setLocalScaling(toBtVector3(scale));
    }

    if (stype == 21) { // trianglemesh
        auto tshpe = (btBvhTriangleMeshShape*)shape;
        tshpe->setLocalScaling(toBtVector3(scale));
    }

    if (stype == 24) { // heightmap, TODO

    }

    if (stype == 31) { // compound
        btCompoundShape* cpshape = (btCompoundShape*)shape;
        cpshape->setLocalScaling(toBtVector3(scale));
    }
}

void VRPhysics::computeAccelerations() {
    if (!isDynamic()) return;
    auto scene = OSG::VRScene::getCurrent();
    if (!scene) return;
    double t = scene->getSimulationTime();
    double dt = t-lastSimTime;
    if (lastSimTime == 0) { lastSimTime = t; return; }
    lastSimTime = t;
    if (dt < 1e-9) return; // 0 or negative..
    double _dt = 1.0/dt;

    Vec3d lv = getLinearVelocity();
    Vec3d av = getAngularVelocity();
    linearAcceleration = (lv-lastLinearVelocity) * _dt;
    angularAcceleration = (av-lastAngularVelocity) * _dt;
    lastLinearVelocity = lv;
    lastAngularVelocity = av;
    //cout << " a " << angularAcceleration << ",  " << _dt << ", " << av << ", " << lastAngularVelocity << ", body: " << toVec3d( bt.body->getAngularVelocity() ) << ", " << vr_obj.lock()->getName() << endl;
}

void VRPhysics::updateTransformation(OSG::VRTransformPtr trans) {
    if (!trans) return;
    VRLock lock(VRPhysics_mtx());
    //static VRRate FPS; int fps = FPS.getRate(); cout << "VRPhysics::updateTransformation " << fps << endl;
    bool scaleChanged = false;
    auto btp = fromVRTransform(trans, scale, CoMOffset, scaleChanged);
    if (scaleChanged) rescaleCollisionShape();
    if (bt.body) { bt.body->setWorldTransform(btp); resetForces(); bt.body->activate(); }
    if (bt.ghost_body) { bt.ghost_body->setWorldTransform(btp); bt.ghost_body->activate(); }
    if (visShape && visShape->isVisible()) visShape->setWorldMatrix( getTransformation() );
}

btTransform VRPhysics::fromVRTransform(OSG::VRTransformPtr trans, OSG::Vec3d& scale, OSG::Vec3d mc, bool& scaleChanged) {
    if (!trans) return fromMatrix(OSG::Matrix4d(),scale,mc,scaleChanged);
    OSG::Matrix4d m = trans->getWorldMatrix();
    return fromMatrix(m,scale,mc,scaleChanged);
}

btTransform VRPhysics::fromMatrix(OSG::Matrix4d m, OSG::Vec3d& scale, OSG::Vec3d mc, bool& scaleChanged) {
    Vec3d lScale = scale;
    for (int i=0; i<3; i++) scale[i] = m[i].length(); // store scale
    if (lScale.dist2(scale) > 1e-5) scaleChanged = true;
    for (int i=0; i<3; i++) m[i] *= 1.0/scale[i]; // normalize
    return fromMatrix(m, mc);
}

btTransform VRPhysics::fromMatrix(OSG::Matrix4d m, OSG::Vec3d mc) {
    OSG::Matrix4d t;
    t.setTranslate(mc); // center of mass offset
    m.mult(t);
    OSG::Matrix4f k = toMatrix4f(m);

    btTransform bltTrans;//Bullets transform
    bltTrans.setFromOpenGLMatrix(&k[0][0]);
    return bltTrans;
}

OSG::Matrix4d VRPhysics::fromBTTransform(const btTransform bt, OSG::Vec3d scale, OSG::Vec3d mc) {
    OSG::Matrix4d m = fromBTTransform(bt);

    OSG::Matrix4d t,s;
    t.setTranslate(-mc); // center of mass offset
    s.setScale(scale); // apply scale

    m.mult(t);
    m.mult(s);
    return m;
}

OSG::Matrix4d VRPhysics::fromBTTransform(const btTransform t) {
    btScalar _m[16];
    t.getOpenGLMatrix(_m);

    OSG::Matrix4d m;
    for (int i=0;i<4;i++) m[0][i] = _m[i];
    for (int i=0;i<4;i++) m[1][i] = _m[4+i];
    for (int i=0;i<4;i++) m[2][i] = _m[8+i];
    for (int i=0;i<4;i++) m[3][i] = _m[12+i];

    //cout << "fromTransform " << m << endl;
    return m;
}

void VRPhysics::pause(bool b) {
    paused = b;
    if (bt.body == 0) return;
    if (!dynamic) return;
    update();
}

void VRPhysics::resetForces() {
    if (bt.body == 0) return;
    VRLock lock(VRPhysics_mtx());
    bt.body->setAngularVelocity(btVector3(0,0,0));
    bt.body->setLinearVelocity(btVector3(0,0,0));
    bt.body->clearForces();
    bt.constantForce = btVector3(0,0,0);
    bt.constantTorque = btVector3(0,0,0);
}

void VRPhysics::applyImpulse(OSG::Vec3d i) {
    if (bt.body == 0) return;
    if (mass == 0) return;
    VRLock lock(VRPhysics_mtx());
    i *= 1.0/mass;
    bt.body->applyCentralImpulse(toBtVector3(i));
}

void VRPhysics::applyTorqueImpulse(OSG::Vec3d i) {
    if (bt.body == 0) return;
    if (mass == 0) return;
    VRLock lock(VRPhysics_mtx());
    //body->setAngularVelocity(btVector3(i[0]/mass, i[1]/mass, i[2]/mass));
    bt.body->applyTorqueImpulse(toBtVector3(i));
}

void VRPhysics::addConstantForce(OSG::Vec3d i) { VRLock lock(VRPhysics_mtx()); bt.constantForce = toBtVector3(i); }
void VRPhysics::addConstantTorque(OSG::Vec3d i) { VRLock lock(VRPhysics_mtx()); bt.constantTorque = toBtVector3(i); }

OSG::Vec3d VRPhysics::getLinearVelocity() {
     if (bt.body == 0) return OSG::Vec3d (0.0f,0.0f,0.0f);
     VRLock lock(VRPhysics_mtx());
     btVector3 tmp = bt.body->getLinearVelocity();
     OSG::Vec3d result = OSG::Vec3d ( tmp.getX(), tmp.getY(), tmp.getZ());
     return result;
}

OSG::Vec3d VRPhysics::getAngularVelocity() {
     if (bt.body == 0) return OSG::Vec3d (0.0f,0.0f,0.0f);
     VRLock lock(VRPhysics_mtx());
     btVector3 tmp = bt.body->getAngularVelocity();
     //btVector3 tmp2 = body->getInterpolationAngularVelocity();
     //cout<<"\n "<<"\n "<< (float)tmp.getX() << "    " <<(float)tmp.getY() <<  "    " <<(float)tmp.getZ() << "\n ";
     //cout<<"\n "<<"\n "<<(float)tmp2.getX() << "    " <<(float)tmp2.getY() <<  "    " <<(float)tmp2.getZ() << "\n ";

     OSG::Vec3d result = OSG::Vec3d (tmp.getX(), tmp.getY(), tmp.getZ());
     return result;
}

Vec3d VRPhysics::getLinearAcceleration() { return linearAcceleration; }
Vec3d VRPhysics::getAngularAcceleration() { return angularAcceleration; }

btTransform VRPhysics::getTransform() {
    if (bt.body == 0) return btTransform();
    btTransform t;

    VRLock lock(VRPhysics_mtx());
    return bt.body->getWorldTransform();
}

OSG::Matrix4d VRPhysics::getTransformation(bool scaled) {
    if (bt.body == 0 && bt.soft_body == 0 && bt.ghost_body == 0) return OSG::Matrix4d();
    btTransform t;
    VRLock lock(VRPhysics_mtx());

    if (bt.body) t = bt.body->getWorldTransform();
    else if (bt.ghost_body) t = bt.ghost_body->getWorldTransform();
    else {
        btSoftBody::tNodeArray& nodes(bt.soft_body->m_nodes);
        btVector3 result = btVector3(0.0,0.0,0.0);
        for(int j=0;j<nodes.size();++j) {
            result += bt.soft_body->m_nodes[j].m_x;
        }
        result /= nodes.size();
        t.setOrigin(result);
    }

    if (scaled) return fromBTTransform(t, scale, CoMOffset);
    else {
        static Vec3d s = Vec3d(1,1,1);
        return fromBTTransform(t, s, CoMOffset);
    }
}

OSG::Matrix4d VRPhysics::getInertiaMoment() {
    btMatrix3x3 bm = getInertiaTensor();
    OSG::Matrix4d m;
    m.setIdentity();
    for (int i=0;i<3;i++) m[0][i] = bm[i][0];
    for (int i=0;i<3;i++) m[1][i] = bm[i][1];
    for (int i=0;i<3;i++) m[2][i] = bm[i][2];
    return m;
}

btMatrix3x3 VRPhysics::getInertiaTensor() {
    if (bt.body == 0) return btMatrix3x3();
    VRLock lock(VRPhysics_mtx());
    bt.body->updateInertiaTensor();
    btMatrix3x3 m = bt.body->getInvInertiaTensorWorld();
    return m.inverse();
}

void VRPhysics::setTransformation(btTransform t) {
    if (bt.body == 0) return;
    VRLock lock(VRPhysics_mtx());
    bt.body->setWorldTransform(t);
}

float VRPhysics::getConstraintAngle(VRPhysics* to, int axis) {
    float ret = 0.0;
    VRLock lock(VRPhysics_mtx());
    if (bt.body) {
        VRPhysicsJoint* joint = joints[to];
        if (joint) {
            ret = joint->btJoint->getAngle(axis);
        }
    }
    return ret;
}

void VRPhysics::deleteConstraints(VRPhysics* with) {
    VRPhysicsJoint* joint = joints[with];
    if (joint != 0) {
        VRLock lock(VRPhysics_mtx());
        bt.world->removeConstraint(joint->btJoint);
    }
}

void VRPhysics::setConstraint(VRPhysics* p,int nodeIndex,OSG::Vec3d localPivot,bool ignoreCollision,float influence) {
    if (bt.soft_body==0) return;
    if (p->bt.body == 0) return;
    VRLock lock(VRPhysics_mtx());
    bt.soft_body->appendAnchor(nodeIndex,p->bt.body,toBtVector3(localPivot),!ignoreCollision,influence);
}

void VRPhysics::setConstraint(VRPhysics* p, OSG::VRConstraintPtr c, OSG::VRConstraintPtr cs, bool disableCollisions) {
    if (bt.body == 0 || p == 0) return;
    if (p->bt.body == 0) return;
    VRLock lock(VRPhysics_mtx());

    if (joints.count(p) == 0) joints[p] = new VRPhysicsJoint(p, c, cs, disableCollisions);
    else {
        joints[p]->constraint = c;
        joints[p]->spring = cs;
    }
    if (p->joints2.count(this) == 0) p->joints2[this] = joints[p];
    updateConstraint(p);
}

void VRPhysics::updateConstraint(VRPhysics* p) {
    if (bt.body == 0 || p == 0) return;
    if (p->bt.body == 0) return;
    if (joints.count(p) == 0) return;

    VRPhysicsJoint* joint = joints[p];
    OSG::VRConstraintPtr c = joint->constraint;
    if (!c) return;

    VRLock lock(VRPhysics_mtx());
    if (joint->btJoint != 0) {
        bt.world->removeConstraint(joint->btJoint);
        delete joint->btJoint;
        joint->btJoint = 0;
    }

     //the two world transforms
    btTransform localA = btTransform();
    localA.setIdentity();
    btTransform localB = btTransform();
    localB.setIdentity();

    //Constraint.getReferenceFrameInB
    localA = fromMatrix( c->getReferenceA()->asMatrix(), -CoMOffset );
    localB = p->fromMatrix( c->getReferenceB()->asMatrix(), -p->CoMOffset );
    joint->btJoint = new btGeneric6DofSpringConstraint(*bt.body, *p->bt.body, localA, localB, true);
    bt.world->addConstraint(joint->btJoint, joint->disableCollisions);

    for (int i=0; i<6; i++) {
        joint->btJoint->setParam(BT_CONSTRAINT_STOP_CFM, 0, i);
        joint->btJoint->setParam(BT_CONSTRAINT_STOP_ERP, 0.6, i);

        double a = c->getMin(i);
        double b = c->getMax(i);
        //if (a <= b) {
            double pi = M_PI;
            if (i == 3) { a = min( max(-pi, a), pi ); b = min( max(-pi, b), pi ); } // clamp to -pi/pi
            if (i == 4) { a = min( max(-pi*0.5, a), pi*0.5 ); b = min( max(-pi*0.5, b), pi*0.5 ); } // clamp to -0.5pi/0.5pi
            if (i == 5) { a = min( max(-pi, a), pi ); b = min( max(-pi, b), pi ); } // clamp to -pi/pi
            joint->btJoint->setLimit(i, a, b);
        //}
    }

    if (auto cs = joint->spring) { // SPRING PARAMETERS
        for (int i=0; i<6; i++) {
            bool b = (cs->getMin(i) > 0);
            float stiffness = cs->getMin(i);
            float damping = cs->getMax(i);
            joint->btJoint->enableSpring(i, b);
            joint->btJoint->setStiffness(i, stiffness);
            joint->btJoint->setDamping(i, damping);
            joint->btJoint->setEquilibriumPoint(i);
        }
    }
}

void VRPhysics::updateConstraints() {
    if (bt.body == 0) return;
    for (auto j : joints) updateConstraint(j.first);
    for (auto j : joints2) j.first->updateConstraint(this);
}

btRigidBody* VRPhysics::getRigidBody() { return bt.getRigidBody(); }
btPairCachingGhostObject* VRPhysics::getGhostBody() { return bt.getGhostBody(); }
btCollisionObject* VRPhysics::getCollisionObject() { return bt.getCollisionObject(); }
btCollisionShape* VRPhysics::getCollisionShape() { return bt.getCollisionShape(); }

void VRPhysics::setSpringParameters(VRPhysics* p, int dof, float stiffnes, float damping) {
    if (bt.body == 0 || p == 0) return;
    if (p->bt.body == 0) return;
    if (joints.count(p) == 0) return;

    VRLock lock(VRPhysics_mtx());
    VRPhysicsJoint* joint = joints[p];
    if (!joint->btJoint) return;
    joint->btJoint->enableSpring(dof, true);
    joint->btJoint->setStiffness(dof, stiffnes);
    joint->btJoint->setDamping(dof, damping);
}

void VRPhysics::setSpringEquilibrium(VRPhysics* p, int dof, float equilibrium) {
    if (bt.body == 0 || p == 0) return;
    if (p->bt.body == 0) return;
    if (joints.count(p) == 0) return;

    VRLock lock(VRPhysics_mtx());
    VRPhysicsJoint* joint = joints[p];
    if (!joint->btJoint) return;
    joint->btJoint->setEquilibriumPoint(dof, equilibrium);
}



