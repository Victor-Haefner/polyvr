#include "CarDynamics.h"
#include "core/scene/VRScene.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/object/VRObjectT.h"
#include "core/math/pose.h"
#include "core/math/path.h"

#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <GL/glut.h>

typedef boost::recursive_mutex::scoped_lock PLock;

using namespace OSG;
using namespace std;

CarDynamics::CarDynamics(string name) : VRObject(name) {
    setPersistency(0);
	initPhysics();
}

CarDynamics::~CarDynamics() {
    return;

	//cleanup in the reverse order of creation/initialization
	//remove the rigidbodies from the dynamics world && delete them
	int i;
	for (i = m_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
		btCollisionObject* obj = m_dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState()) {
			delete body->getMotionState();
		}
		m_dynamicsWorld->removeCollisionObject(obj);
		delete obj;
	}

	//delete collision shapes
	for (int j = 0; j<m_collisionShapes.size(); j++) {
		btCollisionShape* shape = m_collisionShapes[j];
		delete shape;
	}

	//delete dynamics world
	delete m_vehicleRayCaster;
	delete m_vehicle;
}

CarDynamicsPtr CarDynamics::create(string name) { return CarDynamicsPtr( new CarDynamics(name) ); }

//only to be done once
void CarDynamics::initPhysics() {
    auto scene = VRScene::getCurrent();
    updateWPtr = VRFunction<int>::create("cardyn_wheel_update", boost::bind(&CarDynamics::updateWheels, this));
    scene->addUpdateFkt(updateWPtr);
    m_dynamicsWorld = (btDynamicsWorld*) scene->bltWorld();
}

//only to be done once
float CarDynamics::getSpeed() {
    PLock lock(mtx());
    if (!m_vehicle) return 0;
    return m_vehicle->getCurrentSpeedKmHour();
}

float CarDynamics::getAcceleration() { // TODO: idea! get the delta time from the distance traveled!!
    PLock lock(mtx());
    static float last_speed = 0;
    static double last_time = 0;
    float speed = getSpeed();
    double time = glutGet(GLUT_ELAPSED_TIME)*0.001;
    float a = (speed-last_speed)/(time-last_time);
    last_speed = speed;
    last_time = time;
    return a;
}

void CarDynamics::initVehicle() {
    PLock lock(mtx());

    if (chassis.body == 0) { // if no custom chassis set
        cout << "\nINIT with default BOX\n";
        btTransform tr;
        tr.setIdentity();
        tr.setOrigin(btVector3(0, 0, 0));

        btCollisionShape* chassisShape = new btBoxShape(btVector3(1.5f, 1.f, 3.0f));
        m_collisionShapes.push_back(chassisShape);

        btCompoundShape* compound = new btCompoundShape();
        m_collisionShapes.push_back(compound);
        btTransform localTrans;
        localTrans.setIdentity();
        localTrans.setOrigin(btVector3(0, 1, 0)); // localTrans effectively shifts the center of mass with respect to the chassis

        compound->addChildShape(localTrans, chassisShape);
        tr.setOrigin(btVector3(0, 0.f, 0));
        chassis.body = createRigitBody(chassis.mass, tr, compound);
    }

	m_vehicleRayCaster = new btDefaultVehicleRaycaster(m_dynamicsWorld);
	m_vehicle = new btRaycastVehicle(m_tuning, chassis.body, m_vehicleRayCaster);

	//never deactivate the vehicle
	chassis.body->setActivationState(DISABLE_DEACTIVATION);

	// create vehicle
	if(m_dynamicsWorld && m_vehicle){
        m_dynamicsWorld->addVehicle(m_vehicle);
        cout << "\n---vehicle added to the world\n";
	} else cout << "\n!!! problem adding vehicle\n";

	//choose coordinate system
	m_vehicle->setCoordinateSystem(0, 1, 2);

	for (auto& wheel : wheels) {
        btVector3 pos = VRPhysics::toBtVector3(wheel.position);
        btVector3 dir = VRPhysics::toBtVector3(wheel.direction);
        btVector3 axl = VRPhysics::toBtVector3(wheel.axle);
        btWheelInfo& btWheel = m_vehicle->addWheel(pos, dir, axl, wheel.suspensionRestLength, wheel.radius, m_tuning, wheel.isFrontWheel);
        btWheel.m_suspensionStiffness = wheel.suspensionStiffness;
		btWheel.m_wheelsDampingRelaxation = wheel.suspensionDamping;
		btWheel.m_wheelsDampingCompression = wheel.suspensionCompression;
		btWheel.m_frictionSlip = wheel.friction;
		btWheel.m_rollInfluence = wheel.rollInfluence;
	}
}

void CarDynamics::updateWheels() {
    if (!m_vehicle) return;
    PLock lock(mtx());

    for (uint i=0; i<wheels.size(); i++) {
        m_vehicle->updateWheelTransform(i,true);
        auto& wheel = wheels[i];
        if (wheel.geo) {
            auto m = VRPhysics::fromBTTransform(m_vehicle->getWheelInfo(i).m_worldTransform);
            wheel.geo->setWorldMatrix(m);
            wheel.geo->setNoBltFlag();
        }
    }
}

void CarDynamics::setChassisGeo(VRGeometryPtr geo, bool doPhys) {
    geo->setMatrix(Matrix());
    if (doPhys) {
        geo->getPhysics()->setShape("Convex");
        geo->getPhysics()->setMass(chassis.mass);
        geo->getPhysics()->setDynamic(true);
        geo->getPhysics()->setPhysicalized(true);
        geo->getPhysics()->updateTransformation(geo);
    }

    if (geo->getPhysics()->getRigidBody() == 0) {
        cout<<"!!!!!!!!chassis is 0!!!!!!!!\ncreating vehicle with standard parameters && shapes"<<endl;
        initVehicle();
        return;
    }

    {
        PLock lock(mtx());
        chassis.body = geo->getPhysics()->getRigidBody();
    }

    cout << "\nset chassis geo " << geo->getName() << endl;

    initVehicle();
    chassis.geo = geo;
    addChild(geo);
}

VRObjectPtr CarDynamics::getRoot() { return ptr(); }
VRTransformPtr CarDynamics::getChassis() { return chassis.geo; }
vector<VRTransformPtr> CarDynamics::getWheels() {
    vector<VRTransformPtr> res;
    for (auto& wheel : wheels) if (wheel.geo) res.push_back(wheel.geo);
    return res;
}

void CarDynamics::setupSimpleWheels(VRGeometryPtr geo, float x, float fZ, float rZ, float h, float r, float w) {
    xOffset = x;
    frontZOffset = fZ;
    rearZOffset = rZ;
    height = h;

    // create simple wheels
	for (int i=0; i<4; i++) wheels.push_back(Wheel());
	wheels[0].position = Vec3f( xOffset, height, frontZOffset); // front wheel left
	wheels[1].position = Vec3f(-xOffset, height, frontZOffset); // front wheel right
	wheels[2].position = Vec3f( xOffset, height, rearZOffset); // rear wheel right
	wheels[3].position = Vec3f(-xOffset, height, rearZOffset); // rear wheel left
	wheels[0].isFrontWheel = true;
	wheels[1].isFrontWheel = true;

	for (auto& wheel : wheels) {
        wheel.geo = static_pointer_cast<VRGeometry>( geo->duplicate() );
        wheel.geo->setPersistency(0);
        addChild(wheel.geo);
        wheel.width = w;
        wheel.radius = r;
	}
}

void CarDynamics::setThrottle(float t) { // from 0 to 1
    throttle = t;
    t *= engine.power;
    //t = max(0.f,t);
    if (t>0) t = min( engine.maxForce,t);
    else     t = max(-engine.maxForce,t);

    PLock lock(mtx());
    m_vehicle->applyEngineForce(t, 2);
    m_vehicle->applyEngineForce(t, 3);

    //cout << "\nset throttle " << t << endl;
}

void CarDynamics::setBreak(float b) { // from 0 to 1
    breaking = b;
    b *= engine.breakPower;
    b = max(0.f,b);
    b = min(engine.maxBreakingForce,b);

    PLock lock(mtx());
    m_vehicle->setBrake(b, 2);
    m_vehicle->setBrake(b, 3);
}

void CarDynamics::setSteering(float s) { // from -1 to 1
    PLock lock(mtx());

    if (s < -1) s = -1;
    if (s > 1) s = 1;

    steering = s;
    m_vehicle->setSteeringValue(s*engine.maxSteer, 0);
    m_vehicle->setSteeringValue(s*engine.maxSteer, 1);
}

float CarDynamics::getThrottle() { return throttle; }
float CarDynamics::getBreaking() { return breaking; }
float CarDynamics::getSteering() { return steering; }

void CarDynamics::setParameter(float mass, float maxSteering, float enginePower, float breakPower) {
    if (mass > 0) chassis.mass = mass;
    engine.maxSteer = maxSteering;
    engine.power = enginePower;
    engine.breakPower = breakPower;
}

boost::recursive_mutex& CarDynamics::mtx() {
    auto scene = VRScene::getCurrent();
    if (scene) return scene->physicsMutex();
    else {
        static boost::recursive_mutex m;
        return m;
    };
}

void CarDynamics::reset(const pose& p) {
    PLock lock(mtx());

	btTransform t;
	t.setIdentity();
	Matrix m = p.asMatrix();
	t.setFromOpenGLMatrix(&m[0][0]);
	chassis.body->setCenterOfMassTransform(t);
	chassis.body->setLinearVelocity(btVector3(0, 0, 0));
	chassis.body->setAngularVelocity(btVector3(0, 0, 0));
	m_dynamicsWorld->getBroadphase()->getOverlappingPairCache()->cleanProxyFromPairs(chassis.body->getBroadphaseHandle(), m_dynamicsWorld->getDispatcher());
	if (m_vehicle) {
		m_vehicle->resetSuspension();
		for (int i = 0; i<m_vehicle->getNumWheels(); i++) {
			//synchronize the wheels with the (interpolated) chassis worldtransform
			m_vehicle->updateWheelTransform(i, true);
		}
	}

}

btRigidBody* CarDynamics::createRigitBody(float mass, const btTransform& startTransform, btCollisionShape* shape) {
	if (shape == 0) return 0;
    PLock lock(mtx());
	if (shape->getShapeType() == INVALID_SHAPE_PROXYTYPE) return 0;

	btVector3 localInertia(0, 0, 0);
	if (mass != 0.f) shape->calculateLocalInertia(mass, localInertia);

	//using motionstate is recommended, it provides interpolation capabilities, && only synchronizes 'active' objects
	btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo cInfo(mass, myMotionState, shape, localInertia);
	btRigidBody* body = new btRigidBody(cInfo);
	body->setContactProcessingThreshold(BT_LARGE_FLOAT);
    //btRigidBody* body = new btRigidBody(mass, 0, shape, localInertia);
    //body->setWorldTransform(startTransform);
	//dynamicsWorld->addRigidBody(body);
	return body;
}



