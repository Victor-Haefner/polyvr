#include "CarDynamics.h"
#include "CarSound/CarSound.h"
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

VRCarDynamics::VRCarDynamics(string name) : VRObject(name) {
    setPersistency(0);
	initPhysics();
	carSound = CarSound::create();
}

VRCarDynamics::~VRCarDynamics() {
    PLock lock(mtx());
    cout << "\nVRCarDynamics::~VRCarDynamics()\n";
    m_dynamicsWorld->removeVehicle(m_vehicle);
	if (m_vehicle) delete m_vehicle;
	if (m_vehicleRayCaster) delete m_vehicleRayCaster;
}

VRCarDynamicsPtr VRCarDynamics::create(string name) { return VRCarDynamicsPtr( new VRCarDynamics(name) ); }

CarSoundPtr VRCarDynamics::getCarSound() { return carSound; }

//only to be done once
void VRCarDynamics::initPhysics() {
    auto scene = VRScene::getCurrent();
    updateEPtr = VRFunction<int>::create("cardyn_engin_update", boost::bind(&VRCarDynamics::updateEngine, this));
    updateWPtr = VRFunction<int>::create("cardyn_wheel_update", boost::bind(&VRCarDynamics::updateWheels, this));
    scene->addUpdateFkt(updateEPtr);
    scene->addUpdateFkt(updateWPtr);

    PLock lock(mtx());
    m_dynamicsWorld = (btDynamicsWorld*) scene->bltWorld();
	m_vehicleRayCaster = new btDefaultVehicleRaycaster(m_dynamicsWorld);
}

float VRCarDynamics::getSpeed() { return speed; }
float VRCarDynamics::getAcceleration() { return acceleration; }

void VRCarDynamics::updateChassis() {
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

void VRCarDynamics::addBTWheel(Wheel& wheel) {
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

void VRCarDynamics::updateWheels() {
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

void VRCarDynamics::setChassisGeo(VRTransformPtr geo, bool doPhys) {
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

VRObjectPtr VRCarDynamics::getRoot() { return ptr(); }
VRTransformPtr VRCarDynamics::getChassis() { return chassis.geo; }
vector<VRTransformPtr> VRCarDynamics::getWheels() {
    vector<VRTransformPtr> res;
    for (auto& wheel : wheels) if (wheel.geo) res.push_back(wheel.geo);
    return res;
}

void VRCarDynamics::addWheel(VRGeometryPtr geo, Vec3f p, float radius, float width, bool steered, bool driven) {
    Wheel wheel;
    wheel.position = p;
	wheel.isSteered = steered;
	wheel.isDriven = driven;
    wheel.geo = geo;
    wheel.geo->setPersistency(0);
    wheel.width = width;
    wheel.radius = radius;

    addChild(wheel.geo);
    wheels.push_back(wheel);
    addBTWheel(wheel);
}

void VRCarDynamics::setupSimpleWheels(VRTransformPtr geo, float x, float fZ, float rZ, float h, float r, float w) {
    // create four simple wheels
    addWheel(static_pointer_cast<VRGeometry>( geo->duplicate() ), Vec3f( x, h, fZ), r, w, true, true);
    addWheel(static_pointer_cast<VRGeometry>( geo->duplicate() ), Vec3f(-x, h, fZ), r, w, true, true);
    addWheel(static_pointer_cast<VRGeometry>( geo->duplicate() ), Vec3f( x, h, rZ), r, w);
    addWheel(static_pointer_cast<VRGeometry>( geo->duplicate() ), Vec3f(-x, h, rZ), r, w);
}

void VRCarDynamics::setClutch(float t) { // from 0 to 1
    t = max(0.f,t);
    t = min(1.f,t);
    clutch = t;
}

void VRCarDynamics::setThrottle(float t) { // from 0 to 1
    t = max(0.f,t);
    t = min(1.f,t);
    throttle = t;
}

void VRCarDynamics::setBreak(float b) { // from 0 to 1
    b = max(0.f,b);
    b = min(1.f,b);
    breaking = b;
}

void VRCarDynamics::setSteering(float s) { // from -1 to 1
    s = max(-1.f,s);
    s = min( 1.f,s);
    steering = s;
}

void VRCarDynamics::setGear(int g) { engine.gear = g; }

void VRCarDynamics::updateEngine() {
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
    if (abs(eBreak) > abs(eForce)) eForce = 0;

	// apply force wheels
    for (uint i=0; i<wheels.size(); i++) {
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

    carSound->play(engine.rpm);
}

void VRCarDynamics::setIgnition(bool b) {
    engine.running = b;
    engine.rpm = b ? 800 : 0;
}

bool VRCarDynamics::isRunning() { return engine.running; }
float VRCarDynamics::getClutch() { return clutch; }
float VRCarDynamics::getThrottle() { return throttle; }
float VRCarDynamics::getBreaking() { return breaking; }
float VRCarDynamics::getSteering() { return steering; }
int VRCarDynamics::getGear() { return engine.gear; }
int VRCarDynamics::getRPM() { return engine.rpm; }


void VRCarDynamics::setParameter(float mass, float maxSteering, float enginePower, float breakPower, Vec3f massOffset) {
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

boost::recursive_mutex& VRCarDynamics::mtx() {
    auto scene = VRScene::getCurrent();
    if (scene) return scene->physicsMutex();
    else {
        static boost::recursive_mutex m;
        return m;
    };
}

void VRCarDynamics::reset(const pose& p) {
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
btRigidBody* VRCarDynamics::createRigitBody(float mass, const btTransform& startTransform, btCollisionShape* shape) {
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



