#include "VRPhysics.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRConstraint.h"
#include <OpenSG/OSGTriangleIterator.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

struct VRPhysicsJoint {
    OSG::VRConstraint* constraint;
    OSG::VRConstraint* spring;
    VRPhysics* partner;
    btGeneric6DofSpringConstraint* btJoint;


    VRPhysicsJoint() {
        constraint = 0;
        spring = 0;
        partner = 0;
        btJoint = 0;
    }

    ~VRPhysicsJoint() {
        if (btJoint) delete btJoint;
        btJoint = 0;
    }

    VRPhysicsJoint(VRPhysics* p, OSG::VRConstraint* c, OSG::VRConstraint* cs) {
        constraint = c;
        spring = cs;
        partner = p;
        btJoint = 0;
    }
};

VRPhysics::VRPhysics(OSG::VRTransform* t) {
    vr_obj = t;
    world = 0;
    body = 0;
    shape = 0;
    motionState = 0;
    mass = 1.0;
    collisionMargin = 0.04;
    physicalized = false;
    dynamic = false;
    physicsShape = "Convex";
    activation_mode = ACTIVE_TAG;

    collisionGroup = 1;
    collisionMask = 1;
}

VRPhysics::~VRPhysics() {
    if (body) {
        if (world) world->removeRigidBody(body);
        delete body;
    }

    if (shape != 0) delete shape;
    if (motionState != 0) delete motionState;

    for (auto j : joints) {
        world->removeConstraint(j.second->btJoint);
        delete j.second;
    }
}

btCollisionObject* VRPhysics::getCollisionObject() {boost::recursive_mutex::scoped_lock lock(mtx); return ghost ? (btCollisionObject*)ghost_body : (btCollisionObject*)body; }
btRigidBody* VRPhysics::getRigidBody() {boost::recursive_mutex::scoped_lock lock(mtx); return body; }
btPairCachingGhostObject* VRPhysics::getGhostBody() {boost::recursive_mutex::scoped_lock lock(mtx); return ghost_body; }
btCollisionShape* VRPhysics::getCollisionShape() {boost::recursive_mutex::scoped_lock lock(mtx); return shape; }

void VRPhysics::setPhysicalized(bool b) { boost::recursive_mutex::scoped_lock lock(mtx); {physicalized = b;} update(); }
void VRPhysics::setShape(string s, float param) { boost::recursive_mutex::scoped_lock lock(mtx); physicsShape = s; shape_param = param; update(); }
bool VRPhysics::isPhysicalized() {boost::recursive_mutex::scoped_lock lock(mtx); return physicalized; }
string VRPhysics::getShape() {boost::recursive_mutex::scoped_lock lock(mtx); return physicsShape; }
void VRPhysics::setDynamic(bool b) { boost::recursive_mutex::scoped_lock lock(mtx); dynamic = b; update(); }
bool VRPhysics::isDynamic() {boost::recursive_mutex::scoped_lock lock(mtx); return dynamic; }
void VRPhysics::setMass(float m) {boost::recursive_mutex::scoped_lock lock(mtx); mass = m; update(); }
float VRPhysics::getMass() {boost::recursive_mutex::scoped_lock lock(mtx); return mass; }
void VRPhysics::setGravity(OSG::Vec3f v) {boost::recursive_mutex::scoped_lock lock(mtx); body->setGravity(btVector3 (v.x(),v.y(),v.z())); }
void VRPhysics::setCollisionMargin(float m) {boost::recursive_mutex::scoped_lock lock(mtx); collisionMargin = m; update(); }
float VRPhysics::getCollisionMargin() {boost::recursive_mutex::scoped_lock lock(mtx); return collisionMargin; }
void VRPhysics::setCollisionGroup(int g) {boost::recursive_mutex::scoped_lock lock(mtx); collisionGroup = g; update(); }
void VRPhysics::setCollisionMask(int m) {boost::recursive_mutex::scoped_lock lock(mtx); collisionMask = m; update(); }
int VRPhysics::getCollisionGroup() {boost::recursive_mutex::scoped_lock lock(mtx); return collisionGroup; }
int VRPhysics::getCollisionMask() {boost::recursive_mutex::scoped_lock lock(mtx); return collisionMask; }
void VRPhysics::setActivationMode(int m) {boost::recursive_mutex::scoped_lock lock(mtx); activation_mode = m; update(); }
int VRPhysics::getActivationMode() {boost::recursive_mutex::scoped_lock lock(mtx); return activation_mode; }
void VRPhysics::setGhost(bool b) {boost::recursive_mutex::scoped_lock lock(mtx); ghost = b; update(); }
bool VRPhysics::isGhost() {boost::recursive_mutex::scoped_lock lock(mtx); return ghost; }
OSG::Vec3f VRPhysics::toVec3f(btVector3 v) {return OSG::Vec3f(v[0], v[1], v[2]); }
void VRPhysics::setDamping(float lin, float ang) {boost::recursive_mutex::scoped_lock lock(mtx); body->setDamping(btScalar(lin),btScalar(ang)); }
OSG::Vec3f VRPhysics::getForce() {boost::recursive_mutex::scoped_lock lock(mtx); return toVec3f(body->getTotalForce());}
OSG::Vec3f VRPhysics::getTorque() {boost::recursive_mutex::scoped_lock lock(mtx); return toVec3f(body->getTotalTorque());}




vector<VRCollision> VRPhysics::getCollisions() {
    boost::recursive_mutex::scoped_lock lock(mtx);
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
    }
    return shapes;
}

void VRPhysics::update() {
    boost::recursive_mutex::scoped_lock lock(mtx);
    OSG::VRScene* scene = OSG::VRSceneManager::getCurrent();
    if (scene == 0) return;

    if (world == 0) world = scene->bltWorld();
    if (world == 0) return;

    scene->unphysicalize(vr_obj);

    if (body != 0) {
        for (auto j : joints) {
            if (j.second->btJoint != 0) {
                VRPhysicsJoint* joint = j.second;
                world->removeConstraint(joint->btJoint);
                delete joint->btJoint;
                joint->btJoint = 0;
            }
        }

        for (auto j : joints2) {
            if (j.first->joints.count(this) == 0) continue;
            VRPhysicsJoint* joint = j.first->joints[this];
            if (joint->btJoint != 0) {
                world->removeConstraint(joint->btJoint);
                delete joint->btJoint;
                joint->btJoint = 0;
            }
        }

        world->removeRigidBody(body);
        delete body;
        body = 0;
    }

    if (ghost_body != 0) {
        world->removeCollisionObject(ghost_body);
        delete ghost_body;
        ghost_body = 0;
    }

    if (shape != 0) { delete shape; shape = 0; }
    if (motionState != 0) { delete motionState; motionState = 0; }

    if (!physicalized) return;

    motionState = new btDefaultMotionState(fromVRTransform( vr_obj, scale ));

    if (physicsShape == "Box") shape = getBoxShape();
    if (physicsShape == "Sphere") shape = getSphereShape();
    if (physicsShape == "Convex") shape = getConvexShape();
    if (physicsShape == "Concave") shape = getConcaveShape();
    if (shape == 0) return;

    btVector3 inertiaVector(0,0,0);
    float _mass = mass;
    if (!dynamic) _mass = 0;

    if (_mass != 0) shape->calculateLocalInertia(_mass, inertiaVector);

    btRigidBody::btRigidBodyConstructionInfo rbInfo( _mass, motionState, shape, inertiaVector );
    if (ghost) {
        ghost_body = new btPairCachingGhostObject();
        ghost_body->setCollisionShape( shape );
        ghost_body->setUserPointer( vr_obj );
        ghost_body->setCollisionFlags( btCollisionObject::CF_KINEMATIC_OBJECT | btCollisionObject::CF_NO_CONTACT_RESPONSE );
        world->addCollisionObject(ghost_body, collisionGroup, collisionMask);
    } else {
        body = new btRigidBody(rbInfo);
        body->setActivationState(activation_mode);
        world->addRigidBody(body, collisionGroup, collisionMask);
    }

    scene->physicalize(vr_obj);
    updateConstraints();
}




btCollisionShape* VRPhysics::getBoxShape() {
    boost::recursive_mutex::scoped_lock lock(mtx);
    if (shape_param > 0) return new btBoxShape( btVector3(shape_param, shape_param, shape_param) );
    float x,y,z;
    x=y=z=0;

    vector<OSG::VRObject*> geos = vr_obj->getObjectListByType("Geometry");
    for (unsigned int j=0; j<geos.size(); j++) {
        OSG::VRGeometry* geo = (OSG::VRGeometry*)geos[j];
        if (geo == 0) continue;
        OSG::GeoVectorPropertyRecPtr pos = geo->getMesh()->getPositions();
		for (unsigned int i = 0; i<pos->size(); i++) {
            OSG::Pnt3f p;
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
    boost::recursive_mutex::scoped_lock lock(mtx);
    if (shape_param > 0) return new btSphereShape( shape_param );

    float r2 = 0;
    //int N = 0;
    OSG::Pnt3f p;
    OSG::Pnt3f center;

    auto geos = vr_obj->getObjectListByType("Geometry");

    /*for (auto _g : geos ) { // get geometric center // makes no sense as you would have to change the center of the shape..
        OSG::VRGeometry* geo = (OSG::VRGeometry*)_g;
        OSG::GeoVectorPropertyRecPtr pos = geo->getMesh()->getPositions();
        N += pos->size();
        for (unsigned int i=0; i<pos->size(); i++) {
            pos->getValue(p,i);
            center += p;
        }
    }

    center *= 1.0/N;*/

    for (auto _g : geos ) {
        OSG::VRGeometry* geo = (OSG::VRGeometry*)_g;
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

btCollisionShape* VRPhysics::getConvexShape() {
    boost::recursive_mutex::scoped_lock lock(mtx);

    btConvexHullShape* shape = new btConvexHullShape();

    vector<OSG::VRObject*> geos = vr_obj->getObjectListByType("Geometry");
	for (unsigned int j = 0; j<geos.size(); j++) {
        OSG::VRGeometry* geo = (OSG::VRGeometry*)geos[j];
        if (geo == 0) continue;
        if (geo->getMesh() == 0) continue;
        OSG::GeoVectorPropertyRecPtr pos = geo->getMesh()->getPositions();
        if (pos == 0) continue;
		for (unsigned int i = 0; i<pos->size(); i++) {
            OSG::Pnt3f p;
            pos->getValue(p,i);
            for (int i=0; i<3; i++) p[i] *= scale[i];
            shape->addPoint(btVector3(p[0], p[1], p[2]));
        }
    }

    shape->setMargin(collisionMargin);

    //cout << "\nConstruct Convex shape for " << vr_obj->getName() << endl;
    return shape;
}

btCollisionShape* VRPhysics::getConcaveShape() {
    boost::recursive_mutex::scoped_lock lock(mtx);

    btTriangleMesh* tri_mesh = new btTriangleMesh();

    vector<OSG::VRObject*> geos = vr_obj->getObjectListByType("Geometry");
    int N = 0;
	for (unsigned int j = 0; j<geos.size(); j++) {
        OSG::VRGeometry* geo = (OSG::VRGeometry*)geos[j];
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

btTransform VRPhysics::fromVRTransform(OSG::VRTransform* t, OSG::Vec3f& scale) {
    OSG::Matrix m = t->getWorldMatrix();

    for (int i=0; i<3; i++) scale[i] = m[i].length(); // store scale
    for (int i=0; i<3; i++) m[i] *= 1.0/scale[i]; // normalize

    btTransform bltTrans;
    bltTrans.setFromOpenGLMatrix(&m[0][0]);
    return bltTrans;
}

OSG::Matrix VRPhysics::fromBTTransform(const btTransform t, OSG::Vec3f scale) {

    OSG::Matrix m = fromBTTransform(t);

    // apply scale
    OSG::Matrix s;
    s.setScale(scale);
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
    boost::recursive_mutex::scoped_lock lock(mtx);
    body->setAngularVelocity(btVector3(0,0,0));
    body->setLinearVelocity(btVector3(0,0,0));
    body->clearForces();

}

void VRPhysics::applyImpulse(OSG::Vec3f i) {
    if (body == 0) return;
    if (mass == 0) return;
    boost::recursive_mutex::scoped_lock lock(mtx);
    body->setLinearVelocity(btVector3(i[0]/mass, i[1]/mass, i[2]/mass));
}

void VRPhysics::addForce(OSG::Vec3f i) {
   if (body == 0) return;
   if (mass == 0) return;
   boost::recursive_mutex::scoped_lock lock(mtx);
   btVector3 ttlForce = body->getTotalForce();
   btVector3 force = btVector3(i.x(), i.y(), i.z());
   //ttlForce += force;
   body->applyForce(force,btVector3(0.0,0.0,0.0));
}

void VRPhysics::addTorque(OSG::Vec3f i) {
   if (body == 0) return;
   if (mass == 0) return;
   boost::recursive_mutex::scoped_lock lock(mtx);
   btVector3 ttlTorque = btVector3(i.x(), i.y(), i.z());
   body->applyTorque(ttlTorque);
}




OSG::Vec3f VRPhysics::getLinearVelocity() {

     if (body == 0) return OSG::Vec3f (0.0f,0.0f,0.0f);
     boost::recursive_mutex::scoped_lock lock(mtx);
     btVector3 tmp = body->getLinearVelocity();
     OSG::Vec3f result = OSG::Vec3f ( tmp.getX(), tmp.getY(), tmp.getZ());
     return result;
}

OSG::Vec3f VRPhysics::getAngularVelocity() {

     if (body == 0) return OSG::Vec3f (0.0f,0.0f,0.0f);
     boost::recursive_mutex::scoped_lock lock(mtx);
     btVector3 tmp = body->getAngularVelocity();
     //btVector3 tmp2 = body->getInterpolationAngularVelocity();
     //cout<<"\n "<<"\n "<< (float)tmp.getX() << "    " <<(float)tmp.getY() <<  "    " <<(float)tmp.getZ() << "\n ";
     //cout<<"\n "<<"\n "<<(float)tmp2.getX() << "    " <<(float)tmp2.getY() <<  "    " <<(float)tmp2.getZ() << "\n ";

     OSG::Vec3f result = OSG::Vec3f (tmp.getX(), tmp.getY(), tmp.getZ());
     return result;
}




void VRPhysics::updateTransformation(OSG::VRTransform* t) {
    boost::recursive_mutex::scoped_lock lock(mtx);
    if (body) {
        body->setWorldTransform(fromVRTransform(t, scale));
        body->activate();
    }

    if (ghost_body) {
        ghost_body->setWorldTransform(fromVRTransform(t, scale));
        ghost_body->activate();
    }
}



btTransform VRPhysics::getTransform() {
    if (body == 0) return btTransform();
    boost::recursive_mutex::scoped_lock lock(mtx);
    btTransform t;

    body->getMotionState()->getWorldTransform(t);
    return t;
}

OSG::Matrix VRPhysics::getTransformation() {
    if (body == 0) return OSG::Matrix();
    boost::recursive_mutex::scoped_lock lock(mtx);
    btTransform t;
    if (body->getMotionState() == 0) return OSG::Matrix();
    body->getMotionState()->getWorldTransform(t);
    return fromBTTransform(t, scale);
}

btMatrix3x3 VRPhysics::getInertiaTensor() {
    if (body == 0) return btMatrix3x3();
    boost::recursive_mutex::scoped_lock lock(mtx);
    body->updateInertiaTensor();
    btMatrix3x3 m = body->getInvInertiaTensorWorld();
    return m.inverse();
}




void VRPhysics::setTransformation(btTransform t) {
    if (body == 0) return;
    boost::recursive_mutex::scoped_lock lock(mtx);
    body->setWorldTransform(t);
}



float VRPhysics::getConstraintAngle(VRPhysics* to, int axis) {
    boost::recursive_mutex::scoped_lock lock(mtx);
    float ret = 0.0;
    if(body) {
        VRPhysicsJoint* joint = joints[to];
        if(joint) {
        ret = joint->btJoint->getAngle(axis);
        }
    }
    return ret;
}

void VRPhysics::deleteConstraints(VRPhysics* with) {
    boost::recursive_mutex::scoped_lock lock(mtx);
    VRPhysicsJoint* joint =joints[with];
    if(joint != 0) {
        world->removeConstraint(joint->btJoint);
    }

}

btTransform VRPhysics::fromMatrix(const OSG::Matrix& m) {
 btVector3 pos = btVector3(m[3][0], m[3][1], m[3][2]);
 /*btMatrix3x3 mat = btMatrix3x3(m[0][0], m[0][1], m[0][2],
 m[1][0], m[1][1], m[1][2],
 m[2][0], m[2][1], m[2][2]);*/
 btMatrix3x3 mat = btMatrix3x3(m[0][0], m[1][0], m[2][0],
 m[0][1], m[1][1], m[2][1],
 m[0][2], m[1][2], m[2][2]);
 btQuaternion q;
 mat.getRotation(q);

 btTransform bltTrans;//Bullets transform
 bltTrans.setIdentity();
 bltTrans.setOrigin(pos);
 bltTrans.setRotation(q);
 return bltTrans;
}
void VRPhysics::setConstraint(VRPhysics* p, OSG::VRConstraint* c, OSG::VRConstraint* cs) {
    if (body == 0) return;
    if (p->body == 0) return;
    boost::recursive_mutex::scoped_lock lock(mtx);

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
    boost::recursive_mutex::scoped_lock lock(mtx);

    VRPhysicsJoint* joint = joints[p];
    OSG::VRConstraint* c = joint->constraint;
    if (c == 0) return;

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
    OSG::Matrix m = c->getReference();
    localB = fromMatrix(m);

    // TODO: possible bug - p is not valid, may have been deleted!

    //cout << "\nCreate Joint " << fromTransform(body->getWorldTransform())[3] << " " << fromTransform(p->body->getWorldTransform())[3] << endl;
    //btTransform t = p->body->getWorldTransform().inverse();
    //t.mult(t, body->getWorldTransform()); // the position of the first object in the local coords of the second

    joint->btJoint = new btGeneric6DofSpringConstraint(*body, *p->body, localA, localB, false);
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
    boost::recursive_mutex::scoped_lock lock(mtx);
    for (auto j : joints) updateConstraint(j.first);
    for (auto j : joints2) j.first->updateConstraint(this);
}
