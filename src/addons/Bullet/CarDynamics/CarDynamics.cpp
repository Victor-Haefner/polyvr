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
    PLock lock(mtx());
    cout << "\nCarDynamics::~CarDynamics()\n";
    m_dynamicsWorld->removeVehicle(m_vehicle);
	if (m_vehicle) delete m_vehicle;
	if (m_vehicleRayCaster) delete m_vehicleRayCaster;
}

CarDynamicsPtr CarDynamics::create(string name) { return CarDynamicsPtr( new CarDynamics(name) ); }

//only to be done once
void CarDynamics::initPhysics() {
    auto scene = VRScene::getCurrent();
    updateEPtr = VRFunction<int>::create("cardyn_engin_update", boost::bind(&CarDynamics::updateEngine, this));
    updateWPtr = VRFunction<int>::create("cardyn_wheel_update", boost::bind(&CarDynamics::updateWheels, this));
    scene->addUpdateFkt(updateEPtr);
    scene->addUpdateFkt(updateWPtr);

    PLock lock(mtx());
    m_dynamicsWorld = (btDynamicsWorld*) scene->bltWorld();
	m_vehicleRayCaster = new btDefaultVehicleRaycaster(m_dynamicsWorld);
}

float CarDynamics::getSpeed() { return speed; }
float CarDynamics::getAcceleration() { return acceleration; }

void CarDynamics::updateChassis() {
    PLock lock(mtx());
    chassis.geo->getPhysics()->setPhysicalized(false);
    chassis.geo->getPhysics()->setMass(chassis.mass);
    chassis.geo->getPhysics()->setCenterOfMass(chassis.massOffset);
    chassis.geo->getPhysics()->setPhysicalized(true);
    chassis.body = chassis.geo->getPhysics()->getRigidBody();

    m_dynamicsWorld->removeVehicle(m_vehicle);
    if (m_vehicle) delete m_vehicle;
	m_vehicle = new btRaycastVehicle(m_tuning, chassis.body, m_vehicleRayCaster);
	chassis.body->setActivationState(DISABLE_DEACTIVATION); // never deactivate the vehicle
    m_dynamicsWorld->addVehicle(m_vehicle);
	m_vehicle->setCoordinateSystem(0, 1, 2);
	for (auto& wheel : wheels) addBTWheel(wheel);
}

void CarDynamics::addBTWheel(Wheel& wheel) {
    if (!m_vehicle) return;
    btVector3 pos = VRPhysics::toBtVector3(wheel.position - chassis.massOffset);
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

void CarDynamics::setChassisGeo(VRTransformPtr geo, bool doPhys) {
    for ( auto obj : geo->getChildren( true, "Geometry", true ) ) {
        auto geo = dynamic_pointer_cast<VRGeometry>(obj);
        chassis.geos.push_back(geo);
        geo->applyTransformation();
    }

    if (doPhys) {
        PLock lock(mtx());
        geo->getPhysics()->setShape("Convex");
        geo->getPhysics()->setMass(chassis.mass);
        geo->getPhysics()->setDynamic(true);
        geo->getPhysics()->setPhysicalized(true);
        geo->getPhysics()->updateTransformation(geo);
    }

    {
        PLock lock(mtx());
        chassis.body = geo->getPhysics()->getRigidBody();
    }

    chassis.geo = geo;
    addChild(geo);
    updateChassis();
}

VRObjectPtr CarDynamics::getRoot() { return ptr(); }
VRTransformPtr CarDynamics::getChassis() { return chassis.geo; }
vector<VRTransformPtr> CarDynamics::getWheels() {
    vector<VRTransformPtr> res;
    for (auto& wheel : wheels) if (wheel.geo) res.push_back(wheel.geo);
    return res;
}

void CarDynamics::setupSimpleWheels(VRTransformPtr geo, float x, float fZ, float rZ, float h, float r, float w) {
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


void CarDynamics::setParameter(float mass, float maxSteering, float enginePower, float breakPower, Vec3f massOffset) {
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
	for (int i=-1; i<=6; i++) engine.gearRatios[i] *= 3.5*1.6;
	engine.minRpm = 800;
	engine.maxRpm = 6000;

	// update physics
	if (!chassis.geo) return;
    PLock lock(mtx());
    for ( auto geo : chassis.geos ) {
        geo->setMatrix(Matrix());
        auto p = geo->getPoseTo(chassis.geo);
        geo->setFrom( p->pos() - massOffset + chassis.massOffset );
        geo->applyTransformation();
    }
    chassis.massOffset = massOffset;
    updateChassis();
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

// deprecated
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



