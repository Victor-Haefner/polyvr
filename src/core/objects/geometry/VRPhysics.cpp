#include "VRPhysics.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRConstraint.h"
#include <OpenSG/OSGTriangleIterator.h>

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
    physicsShape = "Box";
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

    for (jointItr = joints.begin(); jointItr != joints.end(); jointItr++) {
        world->removeConstraint(jointItr->second->btJoint);
        delete jointItr->second;
    }
}

btRigidBody* VRPhysics::obj() { return body; }

void VRPhysics::setPhysicalized(bool b) { physicalized = b; update(); }
void VRPhysics::setShape(string s) { physicsShape = s; update(); }
bool VRPhysics::isPhysicalized() { return physicalized; }
string VRPhysics::getShape() { return physicsShape; }
void VRPhysics::setDynamic(bool b) { dynamic = b; update(); }
bool VRPhysics::isDynamic() { return dynamic; }
void VRPhysics::setMass(float m) { mass = m; update(); }
float VRPhysics::getMass() { return mass; }
void VRPhysics::setCollisionMargin(float m) { collisionMargin = m; update(); }
float VRPhysics::getCollisionMargin() { return collisionMargin; }
void VRPhysics::setCollisionGroup(int g) { collisionGroup = g; update(); }
void VRPhysics::setCollisionMask(int m) { collisionMask = m; update(); }
int VRPhysics::getCollisionGroup() { return collisionGroup; }
int VRPhysics::getCollisionMask() { return collisionMask; }
void VRPhysics::setActivationMode(int m) { activation_mode = m; update(); }
int VRPhysics::getActivationMode() { return activation_mode; }

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
    OSG::VRScene* scene = OSG::VRSceneManager::getCurrent();
    if (scene == 0) return;

    if (world == 0) world = scene->bltWorld();
    if (world == 0) return;

    if (body != 0) {
        for (jointItr = joints.begin(); jointItr != joints.end(); jointItr++) {
            if (jointItr->second->btJoint != 0) {
                VRPhysicsJoint* joint = jointItr->second;
                world->removeConstraint(joint->btJoint);
                delete joint->btJoint;
                joint->btJoint = 0;
            }
        }

        for (jointItr = joints2.begin(); jointItr != joints2.end(); jointItr++) {
            if (jointItr->first->joints.count(this) == 0) continue;
            VRPhysicsJoint* joint = jointItr->first->joints[this];
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

    if (!physicalized) return;

    if (shape != 0) delete shape;
    if (physicsShape == "Box") shape = getBoxShape();
    if (physicsShape == "Sphere") shape = getSphereShape();
    if (physicsShape == "Convex") shape = getConvexShape();
    if (physicsShape == "Concave") shape = getConcaveShape();

    if (motionState != 0) delete motionState;
    motionState = new btDefaultMotionState(fromMatrix(vr_obj->getWorldMatrix()));

    btVector3 inertiaVector(0,0,0);
    float _mass = mass;
    if (!dynamic) _mass = 0;

    if (_mass != 0) shape->calculateLocalInertia(_mass, inertiaVector);

    btRigidBody::btRigidBodyConstructionInfo rbInfo( _mass, motionState, shape, inertiaVector );
    body = new btRigidBody(rbInfo);
    body->setActivationState(activation_mode);
    world->addRigidBody(body, collisionGroup, collisionMask);

    scene->physicalize(vr_obj);
    updateConstraints();
}


btCollisionShape* VRPhysics::getBoxShape() {
    float x,y,z;
    x=y=z=0;

    vector<OSG::VRObject*> geos = vr_obj->getObjectListByType("Geometry");
    for (uint j=0; j<geos.size(); j++) {
        OSG::VRGeometry* geo = (OSG::VRGeometry*)geos[j];
        if (geo == 0) continue;
        OSG::GeoVectorPropertyRecPtr pos = geo->getMesh()->getPositions();
        for (uint i=0;i<pos->size();i++) {
            OSG::Pnt3f p;
            pos->getValue(p,i);
            x = max(x, p[0]);
            y = max(y, p[1]);
            z = max(z, p[2]);
        }
    }

    //cout << "\nConstruct Box shape for " << vr_obj->getName() << ": " << 2*x << " " << 2*y << " " << 2*z << endl;
    return new btBoxShape(btVector3(2*x,2*y,2*z));
}

btCollisionShape* VRPhysics::getSphereShape() {
    float r2 = 0;

    vector<OSG::VRObject*> geos = vr_obj->getObjectListByType("Geometry");
    for (uint j=0; j<geos.size(); j++) {
        OSG::VRGeometry* geo = (OSG::VRGeometry*)geos[j];
        OSG::GeoVectorPropertyRecPtr pos = geo->getMesh()->getPositions();
        for (uint i=0;i<pos->size();i++) {
            OSG::Pnt3f p;
            pos->getValue(p,i);
            r2 = max(r2, p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);
        }
    }

    return new btSphereShape(sqrt(r2));
}

btCollisionShape* VRPhysics::getConvexShape() {
    btConvexHullShape* shape = new btConvexHullShape();

    //Matrix m = vr_obj->getWorldMatrix();

    vector<OSG::VRObject*> geos = vr_obj->getObjectListByType("Geometry");
    for (uint j=0; j<geos.size(); j++) {
        OSG::VRGeometry* geo = (OSG::VRGeometry*)geos[j];
        if (geo == 0) continue;
        if (geo->getMesh() == 0) continue;
        OSG::GeoVectorPropertyRecPtr pos = geo->getMesh()->getPositions();
        if (pos == 0) continue;
        for (uint i=0;i<pos->size();i++) {
            OSG::Pnt3f p;
            pos->getValue(p,i);
            //m.mult(p,p);
            shape->addPoint(btVector3(p[0], p[1], p[2]));
        }
    }

    shape->setMargin(collisionMargin);

    //cout << "\nConstruct Convex shape for " << vr_obj->getName() << endl;
    return shape;
}

btCollisionShape* VRPhysics::getConcaveShape() {
    btTriangleMesh* tri_mesh = new btTriangleMesh();

    vector<OSG::VRObject*> geos = vr_obj->getObjectListByType("Geometry");
    for (uint j=0; j<geos.size(); j++) {
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

            tri_mesh->addTriangle(vertexPos[0], vertexPos[1], vertexPos[2]);
            ++ti;
        }
    }

    //cout << "\nConstruct Concave shape for " << vr_obj->getName() << endl;
    btBvhTriangleMeshShape* shape = new btBvhTriangleMeshShape(tri_mesh, true);
    return shape;
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

OSG::Matrix VRPhysics::fromTransform(const btTransform t) {
    btScalar _m[16];
    t.getOpenGLMatrix(_m);

    OSG::Matrix m;
    for (int i=0;i<4;i++) m[0][i] = _m[i];
    for (int i=0;i<4;i++) m[1][i] = _m[4+i];
    for (int i=0;i<4;i++) m[2][i] = _m[8+i];
    for (int i=0;i<4;i++) m[3][i] = _m[12+i];
    return m;
}

void VRPhysics::pause(bool b) {
    return;
    if (body == 0) return;
    if (dynamic == !b) return;
    cout << "\nSET PAUSE dyn: " << dynamic << " pause: " << b << endl;
    setDynamic(!b);
}

void VRPhysics::resetForces() {
    if (body == 0) return;
    body->setAngularVelocity(btVector3(0,0,0));
    body->setLinearVelocity(btVector3(0,0,0));
    body->clearForces();
}

void VRPhysics::applyImpulse(OSG::Vec3f i) {
    if (body == 0) return;
    if (mass == 0) return;
    body->setLinearVelocity(btVector3(i[0]/mass, i[1]/mass, i[2]/mass));
}

void VRPhysics::applyForce(OSG::Vec3f i) {
   if (body == 0) return;
   if (mass == 0) return;
   body->applyCentralForce(btVector3(i[0], i[1], i[2]));
}

btVector3 VRPhysics::getForce() {
    return body->getTotalForce();
}

void VRPhysics::updateTransformation(const OSG::Matrix& m) {
    if(body == 0) return;
    body->setWorldTransform(fromMatrix(m));
    body->activate();
}

OSG::Matrix VRPhysics::getTransformation() {
    if (body == 0) return OSG::Matrix();
    btTransform t;
    body->getMotionState()->getWorldTransform(t);
    return fromTransform(t);
}

void VRPhysics::setTransformation(btTransform t) {
    if (body == 0) return;
    body->setWorldTransform(t);
}

void VRPhysics::setConstraint(VRPhysics* p, OSG::VRConstraint* c, OSG::VRConstraint* cs) {
    if (body == 0) return;
    if (p->body == 0) return;

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

    if (joint->btJoint != 0) {
        world->removeConstraint(joint->btJoint);
        delete joint->btJoint;
        joint->btJoint = 0;
    }

    // TODO: possible bug - p is not valid, may have been deleted!

    //cout << "\nCreate Joint " << fromTransform(body->getWorldTransform())[3] << " " << fromTransform(p->body->getWorldTransform())[3] << endl;
    btTransform t = p->body->getWorldTransform().inverse();
    t.mult(t, body->getWorldTransform()); // the position of the first object in the local coords of the second
    joint->btJoint = new btGeneric6DofSpringConstraint(*body, *p->body, btTransform::getIdentity(), t, true);
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

    for (jointItr = joints.begin(); jointItr != joints.end(); jointItr++) {
        updateConstraint(jointItr->first);
    }

    for (jointItr = joints2.begin(); jointItr != joints2.end(); jointItr++) {
        jointItr->first->updateConstraint(this);
    }
}
