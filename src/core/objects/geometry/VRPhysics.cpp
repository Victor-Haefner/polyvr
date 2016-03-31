#include "VRPhysics.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRConstraint.h"
#include "core/utils/VRVisualLayer.h"

#include <OpenSG/OSGTriangleIterator.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>

#include <HACD/hacdCircularList.h>
#include <HACD/hacdVector.h>
#include <HACD/hacdICHull.h>
#include <HACD/hacdGraph.h>
#include <HACD/hacdHACD.h>

#include <ConvexDecomposition/cd_wavefront.h>
#include <ConvexDecomposition/ConvexBuilder.h>

/*

IMPORTANT: ..not compiling? you need to install the libbullet-extras-dev package!
open a terminal
sudo apt-get install libbullet-extras-dev

*/

typedef boost::recursive_mutex::scoped_lock Lock;

struct VRPhysicsJoint {
    OSG::VRConstraint* constraint = 0;
    OSG::VRConstraint* spring = 0;
    VRPhysics* partner = 0;
    btGeneric6DofSpringConstraint* btJoint = 0;

    VRPhysicsJoint() {;}

    ~VRPhysicsJoint() {
        if (btJoint) delete btJoint;
    }

    VRPhysicsJoint(VRPhysics* p, OSG::VRConstraint* c, OSG::VRConstraint* cs) {
        constraint = c;
        spring = cs;
        partner = p;
        btJoint = 0;
    }
};

VRPhysics::VRPhysics(OSG::VRTransformWeakPtr t) {
    vr_obj = t;
    world = 0;
    body = 0;
    shape = 0;
    motionState = 0;
    mass = 1.0;
    scale = OSG::Vec3f(1,1,1);
    collisionMargin = 0.04;
    physicalized = false;
    dynamic = false;
    physicsShape = "Convex";
    activation_mode = ACTIVE_TAG;

    gravity = btVector3(0,-10,0);
    constantForce = btVector3(0,0,0);
    constantTorque = btVector3(0,0,0);

    soft_body = 0;
    soft = false;

    collisionGroup = 1;
    collisionMask = 1;
}

VRPhysics::~VRPhysics() {
    Lock lock(mtx());
    clear();
}

boost::recursive_mutex& VRPhysics::mtx() {
    auto scene = OSG::VRSceneManager::getCurrent();
    if (scene) return scene->physicsMutex();
    else {
        static boost::recursive_mutex m;
        return m;
    };
}


btRigidBody* VRPhysics::getRigidBody() { Lock lock(mtx()); return body; }
btPairCachingGhostObject* VRPhysics::getGhostBody() { Lock lock(mtx()); return ghost_body; }
btCollisionShape* VRPhysics::getCollisionShape() { Lock lock(mtx()); return shape; }

OSG::Vec3f VRPhysics::toVec3f(btVector3 v) { return OSG::Vec3f(v[0], v[1], v[2]); }
btVector3 VRPhysics::toBtVector3(OSG::Vec3f v) { return btVector3(v[0], v[1], v[2]); }

void VRPhysics::setPhysicalized(bool b) { physicalized = b; update(); }
void VRPhysics::setShape(string s, float param) { physicsShape = s; shape_param = param; update(); }
bool VRPhysics::isPhysicalized() { return physicalized; }
string VRPhysics::getShape() { return physicsShape; }
void VRPhysics::setDynamic(bool b) { dynamic = b; update(); }
bool VRPhysics::isDynamic() { return dynamic; }
void VRPhysics::setMass(float m) { mass = m; update(); }
float VRPhysics::getMass() { return mass; }
void VRPhysics::setGravity(OSG::Vec3f v) { gravity = toBtVector3(v); update(); }
void VRPhysics::setCollisionMargin(float m) { collisionMargin = m; update(); }
float VRPhysics::getCollisionMargin() { return collisionMargin; }
void VRPhysics::setCollisionGroup(int g) { collisionGroup = g; update(); }
void VRPhysics::setCollisionMask(int m) { collisionMask = m; update(); }
int VRPhysics::getCollisionGroup() { return collisionGroup; }
int VRPhysics::getCollisionMask() { return collisionMask; }
void VRPhysics::setActivationMode(int m) { activation_mode = m; update(); }
int VRPhysics::getActivationMode() { return activation_mode; }
void VRPhysics::setGhost(bool b) { ghost = b; update(); }
bool VRPhysics::isGhost() { return ghost; }
void VRPhysics::setSoft(bool b) { soft = b; update(); }
bool VRPhysics::isSoft() { return soft; }
void VRPhysics::setDamping(float lin, float ang) { linDamping = lin; angDamping = ang; update(); }
OSG::Vec3f VRPhysics::getForce() { Lock lock(mtx()); return toVec3f(constantForce); }
OSG::Vec3f VRPhysics::getTorque() { Lock lock(mtx()); return toVec3f(constantTorque); }

void VRPhysics::prepareStep() {
    if(soft) return;
    if(body == 0) return;
    auto f = constantForce;
    auto t = constantTorque;
    for (auto j : forceJob) { f += toBtVector3(j); forceJob2.push_back(j); }
    for (auto j : torqueJob) { t += toBtVector3(j); torqueJob2.push_back(j); }
    forceJob.clear();
    torqueJob.clear();
    if (f.length2() > 0) body->applyCentralForce(f);
    if (t.length2() > 0) body->applyTorque(t);
}

btCollisionObject* VRPhysics::getCollisionObject() {
     Lock lock(mtx());
     if(ghost) return (btCollisionObject*)ghost_body;
     if(soft)  return (btCollisionObject*)soft_body;
     else return body;
}

vector<VRCollision> VRPhysics::getCollisions() {
    Lock lock(mtx());
    vector<VRCollision> res;
    if (!physicalized) return res;
    if (!ghost) {
        int numManifolds = world->getDispatcher()->getNumManifolds();
        for (int i=0;i<numManifolds;i++) {
            btPersistentManifold* contactManifold =  world->getDispatcher()->getManifoldByIndexInternal(i);
            //btCollisionObject* obA = (btCollisionObject*)(contactManifold->getBody0());
            //btCollisionObject* obB = (btCollisionObject*)(contactManifold->getBody1());

            int numContacts = contactManifold->getNumContacts();
            for (int j=0;j<numContacts;j++) {
                btManifoldPoint& pt = contactManifold->getContactPoint(j);
                if (pt.getDistance()<0.f) {
                    VRCollision c;
                    c.obj1 = vr_obj;
                    // c.obj2 = // TODO
                    c.pos1 = toVec3f( pt.getPositionWorldOnA() );
                    c.pos2 = toVec3f( pt.getPositionWorldOnB() );
                    c.norm = toVec3f( pt.m_normalWorldOnB );
                    c.distance = pt.getDistance();
                    res.push_back(c);
                }
            }
        }
        return res;
    }

    // --------- ghost object --------------
    btManifoldArray   manifoldArray;
    btBroadphasePairArray& pairArray = ghost_body->getOverlappingPairCache()->getOverlappingPairArray();
    int numPairs = pairArray.size();

    for (int i=0;i<numPairs;i++) {
        manifoldArray.clear();

        const btBroadphasePair& pair = pairArray[i];

        //unless we manually perform collision detection on this pair, the contacts are in the dynamics world paircache:
        btBroadphasePair* collisionPair = world->getPairCache()->findPair(pair.m_pProxy0,pair.m_pProxy1);
        if (!collisionPair)
            continue;

        if (collisionPair->m_algorithm)
            collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);

        for (int j=0;j<manifoldArray.size();j++) {
            btPersistentManifold* manifold = manifoldArray[j];
            btScalar directionSign = manifold->getBody0() == ghost_body ? btScalar(-1.0) : btScalar(1.0);
            for (int p=0;p<manifold->getNumContacts();p++) {
                const btManifoldPoint&pt = manifold->getContactPoint(p);
                if (pt.getDistance()<0.f) {
                    VRCollision c;
                    c.pos1 = toVec3f( pt.getPositionWorldOnA() );
                    c.pos2 = toVec3f( pt.getPositionWorldOnB() );
                    c.norm = toVec3f( pt.m_normalWorldOnB*directionSign );
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

void VRPhysics::setCenterOfMass(OSG::Vec3f com) {
    CoMOffset = com;
    comType = "custom";
    update();
}

void VRPhysics::clear() {
    auto scene = OSG::VRSceneManager::getCurrent();
    if (scene) scene->unphysicalize(vr_obj.lock());

    if (scene) world = scene->bltWorld();
    else world = 0;

    if (body != 0) {
        for (auto j : joints) {
            if (j.second->btJoint != 0) {
                VRPhysicsJoint* joint = j.second;
                if (world) world->removeConstraint(joint->btJoint);
                delete joint->btJoint;
                joint->btJoint = 0;
            }
        }

        for (auto j : joints2) {
            if (j.first->joints.count(this) == 0) continue;
            VRPhysicsJoint* joint = j.first->joints[this];
            if (joint->btJoint != 0) {
                if (world) world->removeConstraint(joint->btJoint);
                delete joint->btJoint;
                joint->btJoint = 0;
            }
        }

        if (world) world->removeRigidBody(body);
        delete body;
        body = 0;
    }

    if (ghost_body != 0) {
        if (world) world->removeCollisionObject(ghost_body);
        delete ghost_body;
        ghost_body = 0;
    }

    if (soft_body != 0) {
        if (world) world->removeCollisionObject(soft_body);
        delete soft_body;
        soft_body = 0;
    }

    if (shape != 0) { delete shape; shape = 0; }
    if (motionState != 0) { delete motionState; motionState = 0; }

    if (visShape) visShape->destroy();
    visShape.reset();
}

void VRPhysics::update() {
    auto scene = OSG::VRSceneManager::getCurrent();
    if (scene == 0) return;

    if (world == 0) world = scene->bltWorld();
    if (world == 0) return;

    Lock lock(mtx());
    clear();

    if (!physicalized) return;


    btVector3 inertiaVector(0,0,0);
    float _mass = mass;
    if (!dynamic) _mass = 0;



    if(soft) {
        if(physicsShape == "Cloth") soft_body = createCloth();
        if(physicsShape == "Rope") soft_body = createRope();
        if (soft_body == 0) { return; }
        soft_body->setActivationState(activation_mode);
        world->addSoftBody(soft_body,collisionGroup, collisionMask);
        scene->physicalize(vr_obj.lock());
        updateConstraints();
        return;
    }

    CoMOffset = OSG::Vec3f(0,0,0);
    if (comType == "custom") CoMOffset = CoMOffset_custom;
    if (physicsShape == "Compound") shape = getCompoundShape();
    if (physicsShape == "Box") shape = getBoxShape();
    if (physicsShape == "Sphere") shape = getSphereShape();
    if (physicsShape == "Convex") shape = getConvexShape(CoMOffset);
    if (physicsShape == "Concave") shape = getConcaveShape();
    if (physicsShape == "ConvexDecomposed") shape = getHACDShape();
    if (shape == 0) { cout << "ERROR: physics shape unknown: " << physicsShape << endl; return; }

    motionState = new btDefaultMotionState( fromVRTransform( vr_obj, scale, CoMOffset ) );

    if (_mass != 0) shape->calculateLocalInertia(_mass, inertiaVector);

    if (ghost) {
        ghost_body = new btPairCachingGhostObject();
        ghost_body->setCollisionShape( shape );
        if ( auto sp = vr_obj.lock() ) ghost_body->setUserPointer( sp.get() );
        ghost_body->setCollisionFlags( btCollisionObject::CF_KINEMATIC_OBJECT | btCollisionObject::CF_NO_CONTACT_RESPONSE );
        world->addCollisionObject(ghost_body, collisionGroup, collisionMask);
    } else {
        btRigidBody::btRigidBodyConstructionInfo rbInfo( _mass, motionState, shape, inertiaVector );
        body = new btRigidBody(rbInfo);
        body->setActivationState(activation_mode);
        body->setDamping(btScalar(linDamping),btScalar(angDamping));
        world->addRigidBody(body, collisionGroup, collisionMask);
        body->setGravity(gravity);
    }

    scene->physicalize(vr_obj.lock());
    updateConstraints();

    visShape = OSG::VRGeometry::create("phys_shape");
    updateVisualGeo();
    scene->getVisualLayer()->addObject(visShape);
}

btSoftBody* VRPhysics::createCloth() {
    auto obj = vr_obj.lock();
    if (!obj) return 0;

    if ( !obj->hasAttachment("geometry") ) { cout << "VRPhysics::createCloth only works on geometries" << endl; return 0; }
    OSG::VRGeometryPtr geo = static_pointer_cast<OSG::VRGeometry>(obj);
    if ( geo->getPrimitive()->getType() != "Plane") { cout << "VRPhysics::createCloth only works on Plane primitives" << endl; return 0; }

    OSG::Matrix m = geo->getMatrix();//get Transformation
    geo->setOrientation(OSG::Vec3f(0,0,-1),OSG::Vec3f(0,1,0));//set orientation to identity. ugly solution.
    geo->setWorldPosition(OSG::Vec3f(0.0,0.0,0.0));
    btSoftBodyWorldInfo* info = OSG::VRSceneManager::getCurrent()->getSoftBodyWorldInfo();

    VRPlane* prim = (VRPlane*)geo->getPrimitive();
    float nx = prim->Nx;
    float ny = prim->Ny;
    //float h = prim->height;
    //float w = prim->width;

    OSG::GeoVectorPropertyRecPtr positions = geo->getMesh()->getPositions();
    vector<btVector3> vertices;
    vector<btScalar> masses;

    OSG::Pnt3f p;
    for(uint i = 0; i < positions->size();i++) { //add all vertices
        positions->getValue(p,i);
        m.mult(p,p);
        vertices.push_back( toBtVector3(OSG::Vec3f(p)) );
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

    return ret;//return the first and only plane....
}


btSoftBody* VRPhysics::createRope() {
   return 0;
}


btCollisionShape* VRPhysics::getBoxShape() {
    auto obj = vr_obj.lock();
    if (!obj) return 0;

    if (shape_param > 0) return new btBoxShape( btVector3(shape_param, shape_param, shape_param) );
    float x,y,z;
    x=y=z=0;

    auto geos = obj->getObjectListByType("Geometry");
    for (unsigned int j=0; j<geos.size(); j++) {
        OSG::VRGeometryPtr geo = static_pointer_cast<OSG::VRGeometry>( geos[j] );
        if (geo == 0) continue;
        OSG::GeoVectorPropertyRecPtr pos = geo->getMesh()->getPositions();
		for (unsigned int i = 0; i<pos->size(); i++) {
            OSG::Vec3f p;
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

    float r2 = 0;
    //int N = 0;
    OSG::Vec3f p;
    OSG::Vec3f center;

    auto geos = obj->getObjectListByType("Geometry");

    /*for (auto _g : geos ) { // get geometric center // makes no sense as you would have to change the center of the shape..
        OSG::VRGeometryPtr geo = (OSG::VRGeometryPtr)_g;
        OSG::GeoVectorPropertyRecPtr pos = geo->getMesh()->getPositions();
        N += pos->size();
        for (unsigned int i=0; i<pos->size(); i++) {
            pos->getValue(p,i);
            center += p;
        }
    }

    center *= 1.0/N;*/

    for (auto _g : geos ) {
        OSG::VRGeometryPtr geo = static_pointer_cast<OSG::VRGeometry>(_g);
        OSG::GeoVectorPropertyRecPtr pos = geo->getMesh()->getPositions();
		for (unsigned int i = 0; i<pos->size(); i++) {
            pos->getValue(p,i);
            //r2 = max( r2, (p-center).squareLength() );
            for (int i=0; i<3; i++) p[i] *= scale[i];
            r2 = max( r2, p[0]*p[0]+p[1]*p[1]+p[2]*p[2] );
        }
    }

    return new btSphereShape(sqrt(r2));
}

btCollisionShape* VRPhysics::getConvexShape(OSG::Vec3f& mc) {
    auto obj = vr_obj.lock();
    if (!obj) return 0;

    OSG::Matrix m;
    OSG::Matrix M = obj->getWorldMatrix();
    M.invert();
    vector<OSG::Vec3f> points;
    auto geos = obj->getObjectListByType("Geometry");

    if (comType == "geometric") mc = OSG::Vec3f(); // center of mass
	for (auto g : geos) {
        OSG::VRGeometryPtr geo = static_pointer_cast<OSG::VRGeometry>(g);
        if (geo == 0) continue;
        if (geo->getMesh() == 0) continue;
        OSG::GeoVectorPropertyRecPtr pos = geo->getMesh()->getPositions();
        if (pos == 0) continue;

        if (geo != obj) {
            m = geo->getWorldMatrix();
            m.multLeft(M);
        }

		for (unsigned int i = 0; i<pos->size(); i++) {
            OSG::Vec3f p;
            pos->getValue(p,i);
            if (geo != obj) m.mult(p,p);
            for (int i=0; i<3; i++) p[i] *= scale[i];
            points.push_back(p);
            if (comType == "geometric") mc += OSG::Vec3f(p);
        }
    }

    if (comType == "geometric") mc *= 1.0/points.size(); // displace around center of mass (approx. geom center)

    btConvexHullShape* shape = new btConvexHullShape();
    for (OSG::Vec3f& p : points) shape->addPoint(btVector3(p[0]-mc[0], p[1]-mc[1], p[2]-mc[2]));
    shape->setMargin(collisionMargin);
    return shape;
}

btCollisionShape* VRPhysics::getConcaveShape() {
    auto obj = vr_obj.lock();
    if (!obj) return 0;

    btTriangleMesh* tri_mesh = new btTriangleMesh();

    auto geos = obj->getObjectListByType("Geometry");
    int N = 0;
	for (unsigned int j = 0; j<geos.size(); j++) {
        OSG::VRGeometryPtr geo = static_pointer_cast<OSG::VRGeometry>(geos[j]);
        if (geo == 0) continue;
        if (geo->getMesh() == 0) continue;
        OSG::TriangleIterator ti(geo->getMesh());

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

    OSG::Matrix m;
    OSG::Matrix M = obj->getWorldMatrix();
    M.invert();
    auto geos = obj->getObjectListByType("Geometry");

	for (auto g : geos) {
        vector<OSG::Vec3f> points;
        OSG::VRGeometryPtr geo = static_pointer_cast<OSG::VRGeometry>(g);
        if (geo == 0) continue;
        if (geo->getMesh() == 0) continue;
        OSG::GeoVectorPropertyRecPtr pos = geo->getMesh()->getPositions();
        if (pos == 0) continue;

        if (geo != obj) {
            m = geo->getWorldMatrix();
            m.multLeft(M);
        }

		for (unsigned int i = 0; i<pos->size(); i++) {
            OSG::Vec3f p;
            pos->getValue(p,i);
            if (geo != obj) m.mult(p,p);
            for (int i=0; i<3; i++) p[i] *= scale[i];
            points.push_back(p);
        }

        btConvexHullShape* child = new btConvexHullShape();
        for (OSG::Vec3f& p : points) child->addPoint(btVector3(p[0], p[1], p[2]));

        child->setMargin(collisionMargin);
        btTransform T;
        T.setIdentity();
        shape->addChildShape(T, child);
    }

    return shape;
}

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

btCollisionShape* VRPhysics::getHACDShape() {
    OSG::VRGeometryPtr obj = dynamic_pointer_cast<OSG::VRGeometry>( vr_obj.lock() );
    if (!obj) { cout << "Warning in getHACDShape: not a geometry!"; return 0; }

    std::vector< HACD::Vec3<HACD::Real> > points;
    std::vector< HACD::Vec3<long> > triangles;

    OSG::GeoVectorPropertyRecPtr positions = obj->getMesh()->getPositions();
    OSG::Pnt3f p;
    for(uint i = 0; i < positions->size();i++) {
        positions->getValue(p,i);
        HACD::Vec3<HACD::Real> vertex(p[0], p[1], p[2]);
        points.push_back(vertex);
    }

    for (OSG::TriangleIterator it = OSG::TriangleIterator(obj->getMesh()); !it.isAtEnd() ;++it) {
        HACD::Vec3<long> triangle(it.getPositionIndex(0), it.getPositionIndex(1), it.getPositionIndex(2));
        triangles.push_back(triangle);
    }

    HACD::HACD myHACD;
    myHACD.SetPoints(&points[0]);
    myHACD.SetNPoints(points.size());
    myHACD.SetTriangles(&triangles[0]);
    myHACD.SetNTriangles(triangles.size());
    myHACD.SetCompacityWeight(0.1);
    myHACD.SetVolumeWeight(0.0);
    myHACD.SetNClusters(2);                       // minimum number of clusters
    myHACD.SetNVerticesPerCH(100);                // max of 100 vertices per convex-hull
    myHACD.SetConcavity(100);                     // maximum concavity
    myHACD.SetAddExtraDistPoints(false);
    myHACD.SetAddNeighboursDistPoints(false);
    myHACD.SetAddFacesPoints(false);
    myHACD.Compute();

    MyConvexDecomposition convexDecomposition;
    btCompoundShape* shape = new btCompoundShape();

    for (int c=0; c < myHACD.GetNClusters(); c++) {
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

        ConvexResult r(nPoints, vertices, nTriangles, triangles);
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
}

OSG::VRTransformPtr VRPhysics::getVisualShape() { return visShape; }

void VRPhysics::updateVisualGeo() {
    auto geo = visShape;
    if (!geo) return;
    btCollisionShape* shape = getCollisionShape();
    int stype = shape->getShapeType();

    // 0 : box
    // 4 : convex
    // 8 : sphere
    // 21 : concave
    // 31 : compound

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
        OSG::Vec3f dim;
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

        OSG::GeoPnt3fPropertyRecPtr pos = OSG::GeoPnt3fProperty::create();
        OSG::GeoVec3fPropertyRecPtr norms = OSG::GeoVec3fProperty::create();
        OSG::GeoUInt32PropertyRecPtr inds = OSG::GeoUInt32Property::create();

        for (int i=0; i<Ni; i++) inds->addValue( bt_inds[i] );
        for (int i=0; i<Nv; i++) {
            OSG::Vec3f p = VRPhysics::toVec3f(verts[i]);
            p += CoMOffset;
            pos->addValue( p );
            p.normalize();
            norms->addValue( p );
        }

        geo->setType(GL_TRIANGLES);
        geo->setPositions(pos);
        geo->setNormals(norms);
        geo->setIndices(inds);
    }

    if (stype == 31) { // compound
        btCompoundShape* cpshape = (btCompoundShape*)shape;
        OSG::GeoPnt3fPropertyRecPtr pos = OSG::GeoPnt3fProperty::create();
        OSG::GeoVec3fPropertyRecPtr norms = OSG::GeoVec3fProperty::create();
        OSG::GeoUInt32PropertyRecPtr inds = OSG::GeoUInt32Property::create();

        int NIoffset = 0;
        for (int j = 0; j < cpshape->getNumChildShapes(); j++) {
            btConvexHullShape* cshape = (btConvexHullShape*)cpshape->getChildShape(j);
            btShapeHull hull(cshape);
            hull.buildHull(cshape->getMargin());

            int Ni = hull.numIndices();
            int Nv = hull.numVertices();
            const unsigned int* bt_inds =   hull.getIndexPointer();
            const btVector3* verts = hull.getVertexPointer();


            for (int i=0; i<Ni; i++) inds->addValue( NIoffset + bt_inds[i] );
            NIoffset += Nv;

            for (int i=0; i<Nv; i++) {
                OSG::Vec3f p = VRPhysics::toVec3f(verts[i]);
                p += CoMOffset;
                pos->addValue( p );
                p.normalize();
                norms->addValue( p );
            }
        }

        geo->setType(GL_TRIANGLES);
        geo->setPositions(pos);
        geo->setNormals(norms);
        geo->setIndices(inds);
    }

    auto mat = OSG::VRMaterial::get("phys_mat");
    geo->setMaterial(mat);
}

void VRPhysics::updateTransformation(OSG::VRTransformWeakPtr t) {
    Lock lock(mtx());
    auto bt = fromVRTransform(t, scale, CoMOffset);
    if (body) { body->setWorldTransform(bt); body->activate(); }
    if (ghost_body) { ghost_body->setWorldTransform(bt); ghost_body->activate(); }
    if (visShape && visShape->isVisible()) visShape->setWorldMatrix( getTransformation() );
}

btTransform VRPhysics::fromVRTransform(OSG::VRTransformWeakPtr t, OSG::Vec3f& scale, OSG::Vec3f mc) {
    OSG::Matrix m;
    if (auto sp = t.lock()) m = sp->getWorldMatrix();
    return fromMatrix(m,scale,mc);
}

btTransform VRPhysics::fromMatrix(OSG::Matrix m, OSG::Vec3f& scale, OSG::Vec3f mc) {
    for (int i=0; i<3; i++) scale[i] = m[i].length(); // store scale
    for (int i=0; i<3; i++) m[i] *= 1.0/scale[i]; // normalize
    return fromMatrix(m, mc);
}

btTransform VRPhysics::fromMatrix(OSG::Matrix m, OSG::Vec3f mc) {
    OSG::Matrix t;
    t.setTranslate(mc); // center of mass offset
    m.mult(t);

    btTransform bltTrans;//Bullets transform
    bltTrans.setFromOpenGLMatrix(&m[0][0]);
    return bltTrans;
}

OSG::Matrix VRPhysics::fromBTTransform(const btTransform bt, OSG::Vec3f& scale, OSG::Vec3f mc) {
    OSG::Matrix m = fromBTTransform(bt);

    OSG::Matrix t,s;
    t.setTranslate(-mc); // center of mass offset
    s.setScale(scale); // apply scale

    m.mult(t);
    m.mult(s);
    return m;
}

OSG::Matrix VRPhysics::fromBTTransform(const btTransform t) {
    btScalar _m[16];
    t.getOpenGLMatrix(_m);

    OSG::Matrix m;
    for (int i=0;i<4;i++) m[0][i] = _m[i];
    for (int i=0;i<4;i++) m[1][i] = _m[4+i];
    for (int i=0;i<4;i++) m[2][i] = _m[8+i];
    for (int i=0;i<4;i++) m[3][i] = _m[12+i];

    //cout << "fromTransform " << m << endl;
    return m;
}

void VRPhysics::pause(bool b) {
    return;
    if (body == 0) return;
    if (dynamic == !b) return;
    setDynamic(!b);
}

void VRPhysics::resetForces() {
    if (body == 0) return;
    Lock lock(mtx());
    body->setAngularVelocity(btVector3(0,0,0));
    body->setLinearVelocity(btVector3(0,0,0));
    body->clearForces();
    constantForce = btVector3();
    constantTorque = btVector3();
}

void VRPhysics::applyImpulse(OSG::Vec3f i) {
    if (body == 0) return;
    if (mass == 0) return;
    Lock lock(mtx());
    i *= 1.0/mass;
    body->setLinearVelocity(toBtVector3(i));
}

void VRPhysics::applyTorqueImpulse(OSG::Vec3f i) {
    if (body == 0) return;
    if (mass == 0) return;
    Lock lock(mtx());
    //body->setAngularVelocity(btVector3(i[0]/mass, i[1]/mass, i[2]/mass));
    body->applyTorqueImpulse(toBtVector3(i));
}

void VRPhysics::addForce(OSG::Vec3f i) {
   if (body == 0 || mass == 0) return;
   Lock lock(mtx());
   forceJob.push_back(i);
}

void VRPhysics::addTorque(OSG::Vec3f i) {
   if (body == 0 || mass == 0) return;
   Lock lock(mtx());
   torqueJob.push_back(i);
}

void VRPhysics::addConstantForce(OSG::Vec3f i) { Lock lock(mtx()); constantForce = toBtVector3(i); cout << constantForce << "\n"; }
void VRPhysics::addConstantTorque(OSG::Vec3f i) { Lock lock(mtx()); constantTorque = toBtVector3(i); }

OSG::Vec3f VRPhysics::getLinearVelocity() {
     if (body == 0) return OSG::Vec3f (0.0f,0.0f,0.0f);
     Lock lock(mtx());
     btVector3 tmp = body->getLinearVelocity();
     OSG::Vec3f result = OSG::Vec3f ( tmp.getX(), tmp.getY(), tmp.getZ());
     return result;
}

OSG::Vec3f VRPhysics::getAngularVelocity() {
     if (body == 0) return OSG::Vec3f (0.0f,0.0f,0.0f);
     Lock lock(mtx());
     btVector3 tmp = body->getAngularVelocity();
     //btVector3 tmp2 = body->getInterpolationAngularVelocity();
     //cout<<"\n "<<"\n "<< (float)tmp.getX() << "    " <<(float)tmp.getY() <<  "    " <<(float)tmp.getZ() << "\n ";
     //cout<<"\n "<<"\n "<<(float)tmp2.getX() << "    " <<(float)tmp2.getY() <<  "    " <<(float)tmp2.getZ() << "\n ";

     OSG::Vec3f result = OSG::Vec3f (tmp.getX(), tmp.getY(), tmp.getZ());
     return result;
}

btTransform VRPhysics::getTransform() {
    if (body == 0) return btTransform();
    btTransform t;

    Lock lock(mtx());
    return body->getWorldTransform();
}

OSG::Matrix VRPhysics::getTransformation() {
    if (body == 0 && soft_body == 0 && ghost_body == 0) return OSG::Matrix();
    btTransform t;
    Lock lock(mtx());

    if (body) t = body->getWorldTransform();
    else if (ghost_body) t = ghost_body->getWorldTransform();
    else {
        btSoftBody::tNodeArray&   nodes(soft_body->m_nodes);
        btVector3 result = btVector3(0.0,0.0,0.0);
        for(int j=0;j<nodes.size();++j) {
            result += soft_body->m_nodes[j].m_x;
        }
        result /= nodes.size();
        t.setOrigin(result);
    }
    return fromBTTransform(t, scale, CoMOffset);
}

btMatrix3x3 VRPhysics::getInertiaTensor() {
    if (body == 0) return btMatrix3x3();
    Lock lock(mtx());
    body->updateInertiaTensor();
    btMatrix3x3 m = body->getInvInertiaTensorWorld();
    return m.inverse();
}

void VRPhysics::setTransformation(btTransform t) {
    if (body == 0) return;
    Lock lock(mtx());
    body->setWorldTransform(t);
}

float VRPhysics::getConstraintAngle(VRPhysics* to, int axis) {
    float ret = 0.0;
    Lock lock(mtx());
    if(body) {
        VRPhysicsJoint* joint = joints[to];
        if(joint) {
        ret = joint->btJoint->getAngle(axis);
        }
    }
    return ret;
}

void VRPhysics::deleteConstraints(VRPhysics* with) {
    VRPhysicsJoint* joint = joints[with];
    if(joint != 0) {
        Lock lock(mtx());
        world->removeConstraint(joint->btJoint);
    }
}

void VRPhysics::setConstraint(VRPhysics* p,int nodeIndex,OSG::Vec3f localPivot,bool ignoreCollision,float influence) {
    if(soft_body==0) return;
    if(p->body == 0) return;
    Lock lock(mtx());
    soft_body->appendAnchor(nodeIndex,p->body,toBtVector3(localPivot),!ignoreCollision,influence);
}

void VRPhysics::setConstraint(VRPhysics* p, OSG::VRConstraint* c, OSG::VRConstraint* cs) {
    if (body == 0) return;
    if (p->body == 0) return;
    Lock lock(mtx());

    if (joints.count(p) == 0) joints[p] = new VRPhysicsJoint(p, c, cs);
    else {
        joints[p]->constraint = c;
        joints[p]->spring = cs;
    }
    if (p->joints2.count(this) == 0) p->joints2[this] = joints[p];
    updateConstraint(p);
}

void VRPhysics::updateConstraint(VRPhysics* p) {
    if (body == 0) return;
    if (p->body == 0) return;
    if (joints.count(p) == 0) return;

    VRPhysicsJoint* joint = joints[p];
    OSG::VRConstraint* c = joint->constraint;
    if (c == 0) return;

    Lock lock(mtx());
    if (joint->btJoint != 0) {
        world->removeConstraint(joint->btJoint);
        delete joint->btJoint;
        joint->btJoint = 0;
    }

     //the two world transforms
    btTransform localA = btTransform();
    localA.setIdentity();
    btTransform localB = btTransform();
    localB.setIdentity();

    //Constraint.getReferenceFrameInB
    localA = fromMatrix( c->getReferenceA(), -CoMOffset );
    localB = p->fromMatrix( c->getReferenceB(), -p->CoMOffset );

    // TODO: possible bug - p is not valid, may have been deleted!

    //cout << "\nCreate Joint " << fromTransform(body->getWorldTransform())[3] << " " << fromTransform(p->body->getWorldTransform())[3] << endl;
    //btTransform t = p->body->getWorldTransform().inverse();
    //t.mult(t, body->getWorldTransform()); // the position of the first object in the local coords of the second

    joint->btJoint = new btGeneric6DofSpringConstraint(*body, *p->body, localA, localB, true);
    world->addConstraint(joint->btJoint, true);

    for (int i=0; i<6; i++) {
        joint->btJoint->setParam(BT_CONSTRAINT_STOP_CFM, 0, i);
        joint->btJoint->setParam(BT_CONSTRAINT_STOP_ERP, 0.6, i);
    }

    joint->btJoint->setLinearLowerLimit(btVector3(c->getMin(0), c->getMin(1), c->getMin(2)));
    joint->btJoint->setLinearUpperLimit(btVector3(c->getMax(0), c->getMax(1), c->getMax(2)));
    joint->btJoint->setAngularLowerLimit(btVector3(c->getMin(3), c->getMin(4), c->getMin(5)));
    joint->btJoint->setAngularUpperLimit(btVector3(c->getMax(3), c->getMax(4), c->getMax(5)));


    // SPRING PARAMETERS

    OSG::VRConstraint* cs = joint->spring;
    if (cs == 0) return;
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

void VRPhysics::updateConstraints() {
    if (body == 0) return;
    for (auto j : joints) updateConstraint(j.first);
    for (auto j : joints2) j.first->updateConstraint(this);
}
