#include "CarDynamics.h"

#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTextureObjChunk.h>

#include "core/scene/VRScene.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/object/VRObjectT.h"
#include "core/math/pose.h"
#include "core/math/path.h"
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>

#include <GL/glut.h>
#include <stdio.h> //printf debugging

typedef boost::recursive_mutex::scoped_lock PLock;

using namespace OSG;
using namespace std;

#define CUBE_HALF_EXTENTS 1
CarDynamics::CarDynamics(string name) : VRObject(name) {
    setPersistency(0);
    m_defaultContactProcessingThreshold = BT_LARGE_FLOAT;
	w1 = 0;
	w2 = 0;
	w3 = 0;
	w4 = 0;

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
    updatePPtr = VRFunction<int>::create("cardyn_pilot_update", boost::bind(&CarDynamics::updatePilot, this));
    scene->addUpdateFkt(updateWPtr);
    scene->addUpdateFkt(updatePPtr);
    m_dynamicsWorld = (btDynamicsWorld*) scene->bltWorld();
}

//only to be done once
float CarDynamics::getSpeed() { PLock lock(mtx()); return m_vehicle->getCurrentSpeedKmHour(); }

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
    //called only if no custom chassis set
    if (m_carChassis == 0) {
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
        //localTrans effectively shifts the center of mass with respect to the chassis
        localTrans.setOrigin(btVector3(0, 1, 0));

        compound->addChildShape(localTrans, chassisShape);

        tr.setOrigin(btVector3(0, 0.f, 0));

        m_carChassis = createRigitBody(m_mass, tr, compound);//chassisShape);
    }

	m_vehicleRayCaster = new btDefaultVehicleRaycaster(m_dynamicsWorld);
	m_vehicle = new btRaycastVehicle(m_tuning, m_carChassis, m_vehicleRayCaster);

	//never deactivate the vehicle
	m_carChassis->setActivationState(DISABLE_DEACTIVATION);

	// create vehicle
	if(m_dynamicsWorld && m_vehicle){
        m_dynamicsWorld->addVehicle(m_vehicle);
        cout << "\n---vehicle added to the world\n";
	} else cout << "\n!!! problem adding vehicle\n";

	bool isFrontWheel = true;

	//choose coordinate system
	m_vehicle->setCoordinateSystem(rightIndex, upIndex, forwardIndex);

	//front wheel left
	btVector3 connectionPointCS0(xOffset, height,frontZOffset);//(to the side of vehicle, height at which to connect, to the front/rear of vehicle)
	m_vehicle->addWheel(connectionPointCS0, wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, m_tuning, isFrontWheel);

	//front wheel right
	connectionPointCS0 = btVector3(-xOffset, height, frontZOffset);
	m_vehicle->addWheel(connectionPointCS0, wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, m_tuning, isFrontWheel);

	isFrontWheel = false;
	//rear wheel right
	connectionPointCS0 = btVector3(xOffset, height,rearZOffset);
	m_vehicle->addWheel(connectionPointCS0, wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, m_tuning, isFrontWheel);

	//rear wheel left
	connectionPointCS0 = btVector3(-xOffset, height,rearZOffset);
	m_vehicle->addWheel(connectionPointCS0, wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, m_tuning, isFrontWheel);

	for (int i = 0; i<m_vehicle->getNumWheels(); i++) {
		btWheelInfo& wheel = m_vehicle->getWheelInfo(i);
		wheel.m_suspensionStiffness = suspensionStiffness;
		wheel.m_wheelsDampingRelaxation = suspensionDamping;
		wheel.m_wheelsDampingCompression = suspensionCompression;
		wheel.m_frictionSlip = wheelFriction;
		wheel.m_rollInfluence = rollInfluence;
	}

    if (!initialBuilt) initialBuilt = true;

    cout << "\n---done with INIT vehicle\n";
}

void CarDynamics::updateWheels() {
    //if (chassis && m_carChassis) chassis->updateFromBullet(m_carChassis->getWorldTransform());
    //if(!initialBuilt) return;
    if (!m_vehicle) return;
    PLock lock(mtx());
    for(int i = 0; i<4;i++) m_vehicle->updateWheelTransform(i,true);

    if (w1) { w1->setWorldMatrix( VRPhysics::fromBTTransform(m_vehicle->getWheelInfo(0).m_worldTransform) ); w1->setNoBltFlag(); }
    if (w2) { w2->setWorldMatrix( VRPhysics::fromBTTransform(m_vehicle->getWheelInfo(1).m_worldTransform) ); w2->setNoBltFlag(); }
    if (w3) { w3->setWorldMatrix( VRPhysics::fromBTTransform(m_vehicle->getWheelInfo(2).m_worldTransform) ); w3->setNoBltFlag(); }
    if (w4) { w4->setWorldMatrix( VRPhysics::fromBTTransform(m_vehicle->getWheelInfo(3).m_worldTransform) ); w4->setNoBltFlag(); }
}

void CarDynamics::setChassisGeo(VRGeometryPtr geo, bool doPhys) {
    geo->setMatrix(Matrix());
    if (doPhys) {
        geo->getPhysics()->setShape("Convex");
        geo->getPhysics()->setMass(m_mass);
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
        m_carChassis = geo->getPhysics()->getRigidBody();
    }

    cout << "\nset chassis geo " << geo->getName() << endl;

    initVehicle();
    chassis = geo;
    addChild(geo);
}

void CarDynamics::setWheelGeo(VRGeometryPtr geo) { // TODO
    if (!geo) return;
    w1 = static_pointer_cast<VRGeometry>( geo->duplicate() );
    w2 = static_pointer_cast<VRGeometry>( geo->duplicate() );
    w3 = static_pointer_cast<VRGeometry>( geo->duplicate() );
    w4 = static_pointer_cast<VRGeometry>( geo->duplicate() );
    w1->setPersistency(0);
    w2->setPersistency(0);
    w3->setPersistency(0);
    w4->setPersistency(0);

    addChild(w1);
    addChild(w2);
    addChild(w3);
    addChild(w4);

    cout << "\nset wheel geo " << geo->getName() << endl;
}

VRObjectPtr CarDynamics::getRoot() { return ptr(); }
VRTransformPtr CarDynamics::getChassis() { return chassis; }
vector<VRTransformPtr> CarDynamics::getWheels() {
    vector<VRTransformPtr> res;
    res.push_back(w1);
    res.push_back(w2);
    res.push_back(w3);
    res.push_back(w4);
    return res;
}

void CarDynamics::setWheelOffsets(float x, float fZ, float rZ, float h) {
    xOffset = x;
    frontZOffset = fZ;
    rearZOffset = rZ;
    height = h;
}

void CarDynamics::setWheelParams(float w, float r) {
    wheelWidth = w;
    wheelRadius = r;
}

void CarDynamics::setThrottle(float t) { // from 0 to 1
    if (doPilot) return;
    throttle = t;
    t *= enginePower;
    //t = max(0.f,t);
    if (t>0) t = min(maxEngineForce,t);
    else     t = max(-maxEngineForce,t);

    PLock lock(mtx());
    m_vehicle->applyEngineForce(t, 2);
    m_vehicle->applyEngineForce(t, 3);

    //cout << "\nset throttle " << t << endl;
}

void CarDynamics::setBreak(float b) { // from 0 to 1
    if (doPilot) return;
    breaking = b;
    b *= breakPower;
    b = max(0.f,b);
    b = min(maxBreakingForce,b);

    PLock lock(mtx());
    m_vehicle->setBrake(b, 2);
    m_vehicle->setBrake(b, 3);
}

void CarDynamics::setSteering(float s) { // from -1 to 1
    if (doPilot) return;
    PLock lock(mtx());

    if (s < -1) s = -1;
    if (s > 1) s = 1;

    steering = s;
    m_vehicle->setSteeringValue(s*max_steer, 0);
    m_vehicle->setSteeringValue(s*max_steer, 1);
}

float CarDynamics::getThrottle() { return throttle; }
float CarDynamics::getBreaking() { return breaking; }
float CarDynamics::getSteering() { return steering; }

void CarDynamics::setCarMass(float m) { if (m > 0) m_mass = m; }

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

	gVehicleSteering = 0.f;
	btTransform t;
	t.setIdentity();
	Matrix m = p.asMatrix();
	t.setFromOpenGLMatrix(&m[0][0]);
	m_carChassis->setCenterOfMassTransform(t);
	m_carChassis->setLinearVelocity(btVector3(0, 0, 0));
	m_carChassis->setAngularVelocity(btVector3(0, 0, 0));
	m_dynamicsWorld->getBroadphase()->getOverlappingPairCache()->cleanProxyFromPairs(m_carChassis->getBroadphaseHandle(), m_dynamicsWorld->getDispatcher());
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
	body->setContactProcessingThreshold(m_defaultContactProcessingThreshold);
    //btRigidBody* body = new btRigidBody(mass, 0, shape, localInertia);
    //body->setWorldTransform(startTransform);
	//dynamicsWorld->addRigidBody(body);
	return body;
}

void CarDynamics::updatePilot() {
    if (!doPilot) return;
    if (!p_path || !v_path) return;

    auto clamp = [&](float& v, float a, float b) {
        if (v < a) v = a;
        if (v > b) v = b;
    };

    auto cpose = chassis->getPose();
    auto pos = cpose->pos();
    auto dir = cpose->dir();
    auto up = cpose->up();
    auto speed = getSpeed();

    float t = p_path->getClosestPoint( pos ); // get closest path point
    float L = p_path->getLength();
    float aimingLength = 2.0*speed;

    Vec3f p0 = p_path->getPose(t).pos();
    t += aimingLength/L; // aim some meter ahead
    clamp(t,0,1);

    auto tpos = p_path->getPose(t).pos(); // get target position
    auto tvel = v_path->getPose(t).pos()[1]; // get target velocity

    float target_speed = 5; // TODO: get from path

    // compute throttle and breaking
    float sDiff = target_speed-speed;
    throttle = 0;
    breaking = 0;
    if (sDiff > 0) throttle = sDiff*0.1;
    if (sDiff < 0) breaking = sDiff*0.1;

    // compute steering
    Vec3f delta = tpos - pos;
    delta.normalize();
    dir.normalize();
    up.normalize();
    Vec3f w = delta.cross(dir);
    steering = w.dot(up)*2.0;

    // clamp inputs
    clamp(throttle, 0,1);
    clamp(breaking, 0,1);
    clamp(steering, -1,1);

    // apply inputs
    doPilot = false;
    setThrottle(throttle);
    setBreak(breaking);
    setSteering(steering);
    doPilot = true;
}

void CarDynamics::followPath(pathPtr p, pathPtr v) {
    p_path = p;
    v_path = v;
    doPilot = true;
}

void CarDynamics::stopPilot() { doPilot = false; }
bool CarDynamics::onAutoPilot() { return doPilot; }



