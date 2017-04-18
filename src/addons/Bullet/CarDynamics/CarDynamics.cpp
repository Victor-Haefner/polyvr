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
#include <math.h>

typedef boost::recursive_mutex::scoped_lock PLock;

using namespace OSG;
using namespace std;

CarDynamics::CarDynamics(string name) : VRObject(name) {
    setPersistency(0);
	initPhysics();
}

CarDynamics::~CarDynamics() {
    cout << "\nCarDynamics::~CarDynamics()\n";
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
	m_vehicle = 0;
}

CarDynamicsPtr CarDynamics::create(string name) { return CarDynamicsPtr( new CarDynamics(name) ); }

//only to be done once
void CarDynamics::initPhysics() {
    auto scene = VRScene::getCurrent();
    updateEPtr = VRFunction<int>::create("cardyn_engin_update", boost::bind(&CarDynamics::updateEngine, this));
    updateWPtr = VRFunction<int>::create("cardyn_wheel_update", boost::bind(&CarDynamics::updateWheels, this));
    scene->addUpdateFkt(updateEPtr);
    scene->addUpdateFkt(updateWPtr);
    m_dynamicsWorld = (btDynamicsWorld*) scene->bltWorld();
}

float CarDynamics::getSpeed() { return speed; }
float CarDynamics::getAcceleration() { return acceleration; }

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
        //localTrans.setOrigin(btVector3(0, 1, 0)); // localTrans effectively shifts the center of mass with respect to the chassis
        localTrans.setOrigin( VRPhysics::toBtVector3(chassis.centerOfMass) ); // localTrans effectively shifts the center of mass with respect to the chassis

        compound->addChildShape(localTrans, chassisShape);
        tr.setOrigin(btVector3(0, 0.f, 0));
        chassis.body = createRigitBody(chassis.mass, tr, compound);
    }

	m_vehicleRayCaster = new btDefaultVehicleRaycaster(m_dynamicsWorld);
	m_vehicle = new btRaycastVehicle(m_tuning, chassis.body, m_vehicleRayCaster);

	//never deactivate the vehicle
	chassis.body->setActivationState(DISABLE_DEACTIVATION);

	// create vehicle
	if (m_dynamicsWorld && m_vehicle){
        m_dynamicsWorld->addVehicle(m_vehicle);
        cout << "\n---vehicle added to the world\n";
	} else cout << "\n!!! problem adding vehicle\n";

	//choose coordinate system
	m_vehicle->setCoordinateSystem(0, 1, 2);
	for (auto& wheel : wheels) addBTWheel(wheel);
}

void CarDynamics::addBTWheel(Wheel& wheel) {
    if (!m_vehicle) return;
    btVector3 pos = VRPhysics::toBtVector3(wheel.position);
    btVector3 dir = VRPhysics::toBtVector3(wheel.direction);
    btVector3 axl = VRPhysics::toBtVector3(wheel.axle);
    btWheelInfo& btWheel = m_vehicle->addWheel(pos, dir, axl, wheel.suspensionRestLength, wheel.radius, m_tuning, wheel.isSteered);
    btWheel.m_suspensionStiffness = wheel.suspensionStiffness;
    btWheel.m_wheelsDampingRelaxation = wheel.suspensionDamping;
    btWheel.m_wheelsDampingCompression = wheel.suspensionCompression;
    btWheel.m_frictionSlip = wheel.friction;
    btWheel.m_rollInfluence = wheel.rollInfluence;
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
	wheels[0].isSteered = true;
	wheels[1].isSteered = true;
	wheels[0].isDriven = true;
	wheels[1].isDriven = true;

	for (auto& wheel : wheels) {
        wheel.geo = static_pointer_cast<VRGeometry>( geo->duplicate() );
        wheel.geo->setPersistency(0);
        addChild(wheel.geo);
        wheel.width = w;
        wheel.radius = r;
        addBTWheel(wheel);
	}
}

void CarDynamics::setClutch(float t) { // from 0 to 1
    t = max(0.f,t);
    t = min(1.f,t);
    clutch = t;
}

void CarDynamics::setThrottle(float t) { // from 0 to 1
    t = max(0.f,t);
    t = min(1.f,t);
    throttle = t;
}

void CarDynamics::setBreak(float b) { // from 0 to 1
    b = max(0.f,b);
    b = min(1.f,b);
    breaking = b;
}

void CarDynamics::setSteering(float s) { // from -1 to 1
    s = max(-1.f,s);
    s = min( 1.f,s);
    steering = s;
}

void CarDynamics::setGear(int g) { engine.gear = g; }

void CarDynamics::updateEngine() {
    if (!m_vehicle) return;
    if (!wheels.size()) return;
    PLock lock(mtx());

    /* input variables

        clutch
        throttle
        breaking
        steering

        engine
    */

    auto clamp = [](float v, float a, float b) {
        if (v < a) return a;
        if (v > b) return b;
        return v;
    };

    // stretch throttle range
	float tmin = 0.1;
	float tmax = 0.9;
    float clampedThrottle = (clamp(throttle,tmin,tmax)-tmin)/(tmax-tmin);

    // compute gears
    float clutchForce = 1;
    if (engine.clutchForceCurve) clutchForce = engine.clutchForceCurve->getPosition(clutch)[1];

    float gearRatio = engine.gearRatios[engine.gear];

    // compute RPM
	float s = abs( getSpeed() ) * 1000/60; // from km/h to m/min
	float coupling = (engine.gear != 0)*clutchForce;
	float wheelRPM = s / (wheels[0].radius * 2 * Pi);
	float wheelERPM = wheelRPM * abs(gearRatio);
	float deltaRPM = ( wheelERPM - engine.rpm ) * coupling;
	float eRPMrange = engine.maxRpm - engine.minRpm;
	float throttleRPM = (engine.maxRpm - engine.rpm) * clampedThrottle;
	if (wheelERPM > engine.maxRpm) gearRatio = 0;

	// compute engine breaking
	float engineFriction = (engine.rpm - engine.minRpm) / eRPMrange * max(deltaRPM*0.005 + 5, 0.0) * (1.0 - clampedThrottle);
	float eBreak = breaking*engine.breakPower + max(engineFriction, 0.f);
	//cout << "throttleRPM " << throttleRPM << " clampedThrottle " << clampedThrottle << " throttleMinRPM " << throttleMinRPM << " engine.rpm " << engine.rpm << endl;
	//cout << "eBreak " << eBreak << " engineFriction " << engineF << " deltaRPM " << deltaRPM << " engine.rpm " << engine.rpm << endl;
	engine.rpm += 0.1 * throttleRPM * engine.running;
	engine.rpm -= 14 * engineFriction * engine.running;
	engine.rpm += 0.1 * deltaRPM;

	if (engine.rpm < engine.minRpm*0.6) setIgnition(false);

	// compute engine force
	float engineF = max( -deltaRPM*0.001f, 0.f); // try to keep the minRPM
    float eForce = clamp(clampedThrottle + engineF, 0, 1) * gearRatio * engine.power * coupling * engine.running;

	// apply force wheels
    for (int i=0; i<wheels.size(); i++) {
        auto& wheel = wheels[i];
        m_vehicle->setBrake(eBreak, i);

        if (wheel.isDriven) {
            m_vehicle->applyEngineForce(eForce, i);
        }

        if (wheel.isSteered) {
            m_vehicle->setSteeringValue(steering*engine.maxSteer, i);
        }
    }

    speed = m_vehicle->getCurrentSpeedKmHour();
    double time = glutGet(GLUT_ELAPSED_TIME)*0.001;
    double dt = time-a_measurement_t;
    if (dt > 0) {
        float a = (speed-s_measurement)/dt;
        s_measurement = speed;
        a_measurement_t = time;
        acceleration = a;//abs(a);
    }
}

void CarDynamics::setIgnition(bool b) {
    engine.running = b;
    engine.rpm = b ? 800 : 0;
}

bool CarDynamics::isRunning() { return engine.running; }
float CarDynamics::getClutch() { return clutch; }
float CarDynamics::getThrottle() { return throttle; }
float CarDynamics::getBreaking() { return breaking; }
float CarDynamics::getSteering() { return steering; }
int CarDynamics::getGear() { return engine.gear; }
int CarDynamics::getRPM() { return engine.rpm; }


void CarDynamics::setParameter(float mass, float maxSteering, float enginePower, float breakPower) {
    if (mass > 0) chassis.mass = mass;
    engine.maxSteer = maxSteering;
    engine.power = enginePower;
    engine.breakPower = breakPower;

    //TODO: pass it
    if (!engine.clutchForceCurve) engine.clutchForceCurve = path::create();
    engine.clutchForceCurve->clear();
    engine.clutchForceCurve->addPoint( pose(Vec3f(0,1,0), Vec3f(1,0,0)));
    engine.clutchForceCurve->addPoint( pose(Vec3f(1,0,0), Vec3f(1,0,0)));
    engine.clutchForceCurve->compute(32);

	engine.gearRatios.clear();
	engine.gearRatios[-1] = -3.5;
	engine.gearRatios[0] = 0;
	engine.gearRatios[1] = 3.5;
	engine.gearRatios[2] = 1.5;
	engine.gearRatios[3] = 0.95;
	engine.gearRatios[4] = 0.75;
	engine.gearRatios[5] = 0.63;
	engine.gearRatios[6] = 0.5;
	for (int i=-1; i<=6; i++) engine.gearRatios[i] *= 3.5;
	engine.minRpm = 800;
	engine.maxRpm = 6000;
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
    setIgnition(false);
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



