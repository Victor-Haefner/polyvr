#include "CarDynamics.h"
#include "CarSound/CarSound.h"
#include "core/scene/VRScene.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/object/VRObjectT.h"
#include "core/math/pose.h"
#include "core/math/path.h"
#include "core/utils/VRStorage_template.h"

#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <GL/glut.h>
#include <math.h>

typedef boost::recursive_mutex::scoped_lock PLock;

using namespace OSG;
using namespace std;


VRCarDynamics::Wheel::Wheel() {
    // suspension parameter
    store("position", &position);
    store("direction", &direction);
    store("axle", &axle);
    store("suspensionRestLength", &suspensionRestLength);
    store("suspensionStiffness", &suspensionStiffness);
    store("suspensionDamping", &suspensionDamping);
    store("suspensionCompression", &suspensionCompression);
    store("rollInfluence", &rollInfluence);
    store("maxSteer", &maxSteer);
    store("isSteered", &isSteered);
    store("isDriven", &isDriven);

    // wheel parameter
    store("friction", &friction);
    store("radius", &radius);
    store("width", &width);
}

VRCarDynamics::Engine::Engine() {
    store("power", &power);
    store("breakPower", &breakPower);
    store("friction", &friction);
    store("frictionCoefficient", &frictionCoefficient);
    store("maxForce", &maxForce);
    store("maxBreakingForce", &maxBreakingForce);
    store("minRpm", &minRpm);
    store("maxRpm", &maxRpm);
    store("stallRpm", &stallRpm);
    storeMap("gearRatios", gearRatios);
    storeObj("clutchTransmissionCurve", clutchTransmissionCurve);
    storeObj("torqueCurve", torqueCurve);
    store("running", &running);
}

VRCarDynamics::Chassis::Chassis() {
    store("mass", &mass);
    store("massOffset", &massOffset);
}

VRCarDynamics::WheelPtr VRCarDynamics::Wheel::create() { return WheelPtr( new Wheel() ); }
VRCarDynamics::ChassisPtr VRCarDynamics::Chassis::create() { return ChassisPtr( new Chassis() ); }
VRCarDynamics::EnginePtr VRCarDynamics::Engine::create() { return EnginePtr( new Engine() ); }

VRCarDynamics::VRCarDynamics(string name) : VRObject(name) {
    engine = Engine::create();
    chassis = Chassis::create();

    setPersistency(0);
	initPhysics();
	carSound = CarSound::create();

    store("type", &type);
    storeObj("engine", engine);
    storeObj("chassis", chassis);
    storeObjVec("wheels", wheels, true);
}

VRCarDynamics::~VRCarDynamics() {
    PLock lock(mtx());
    cout << "\nVRCarDynamics::~VRCarDynamics()\n";
    m_dynamicsWorld->removeVehicle(vehicle);
	if (vehicle) delete vehicle;
	if (vehicleRayCaster) delete vehicleRayCaster;
}

VRCarDynamicsPtr VRCarDynamics::create(string name) { return VRCarDynamicsPtr( new VRCarDynamics(name) ); }

CarSoundPtr VRCarDynamics::getCarSound() { return carSound; }

//only to be done once
void VRCarDynamics::initPhysics() {
    auto scene = VRScene::getCurrent();
    updateEPtr = VRUpdateCb::create("cardyn_engin_update", boost::bind(&VRCarDynamics::updateEngine, this));
    updateWPtr = VRUpdateCb::create("cardyn_wheel_update", boost::bind(&VRCarDynamics::updateWheelGeos, this));
    scene->addUpdateFkt(updateEPtr);
    scene->addUpdateFkt(updateWPtr);

    PLock lock(mtx());
    m_dynamicsWorld = (btDynamicsWorld*) scene->bltWorld();
	vehicleRayCaster = new btDefaultVehicleRaycaster(m_dynamicsWorld);
}

float VRCarDynamics::getSpeed() { return speed; }
float VRCarDynamics::getAcceleration() { return acceleration; }

void VRCarDynamics::updateChassis() {
    PLock lock(mtx());
    chassis->geo->getPhysics()->setPhysicalized(false);
    chassis->geo->getPhysics()->setMass(chassis->mass);
    chassis->geo->getPhysics()->setCenterOfMass(chassis->massOffset);
    chassis->geo->getPhysics()->setPhysicalized(true);
    chassis->body = chassis->geo->getPhysics()->getRigidBody();

    m_dynamicsWorld->removeVehicle(vehicle);
    if (vehicle) delete vehicle;
	vehicle = new btRaycastVehicle(m_tuning, chassis->body, vehicleRayCaster);
	chassis->body->setActivationState(DISABLE_DEACTIVATION); // never deactivate the vehicle
    m_dynamicsWorld->addVehicle(vehicle);
	vehicle->setCoordinateSystem(0, 1, 2);
	for (auto wheel : wheels) addBTWheel(wheel);
}

void VRCarDynamics::addBTWheel(WheelPtr wheel) {
    if (!vehicle) return;
    btVector3 pos = VRPhysics::toBtVector3(wheel->position - chassis->massOffset);
    btVector3 dir = VRPhysics::toBtVector3(wheel->direction);
    btVector3 axl = VRPhysics::toBtVector3(wheel->axle);
    btWheelInfo& btWheel = vehicle->addWheel(pos, dir, axl, wheel->suspensionRestLength, wheel->radius, m_tuning, wheel->isSteered);
    btWheel.m_suspensionStiffness = wheel->suspensionStiffness;
    btWheel.m_wheelsDampingRelaxation = wheel->suspensionDamping;
    btWheel.m_wheelsDampingCompression = wheel->suspensionCompression;
    btWheel.m_frictionSlip = wheel->friction-0.2; //Felix: changed to -0.2
    btWheel.m_rollInfluence = wheel->rollInfluence;
}

void VRCarDynamics::updateWheelGeos() {
    if (!vehicle) return;
    PLock lock(mtx());

    for (uint i=0; i<wheels.size(); i++) {
        vehicle->updateWheelTransform(i,true);
        auto& wheel = wheels[i];
        if (wheel->geo) {
            auto m = VRPhysics::fromBTTransform(vehicle->getWheelInfo(i).m_worldTransform);
            wheel->geo->setWorldMatrix(m);
            wheel->geo->setNoBltFlag();
        }
    }
}

void VRCarDynamics::setChassisGeo(VRTransformPtr geo, bool doPhys) {
    for ( auto obj : geo->getChildren( true, "Geometry", true ) ) {
        auto geo = dynamic_pointer_cast<VRGeometry>(obj);
        chassis->geos.push_back(geo);
        geo->applyTransformation();
    }

    if (doPhys) {
        PLock lock(mtx());
        geo->getPhysics()->setShape("Convex");
        geo->getPhysics()->setMass(chassis->mass);
        geo->getPhysics()->setDynamic(true);
        geo->getPhysics()->setPhysicalized(true);
        geo->getPhysics()->updateTransformation(geo);
    }

    {
        PLock lock(mtx());
        chassis->body = geo->getPhysics()->getRigidBody();
    }

    chassis->geo = geo;
    addChild(geo);
    updateChassis();
}

VRObjectPtr VRCarDynamics::getRoot() { return ptr(); }
VRTransformPtr VRCarDynamics::getChassis() { return chassis->geo; }
vector<VRTransformPtr> VRCarDynamics::getWheels() {
    vector<VRTransformPtr> res;
    for (auto& wheel : wheels) if (wheel->geo) res.push_back(wheel->geo);
    return res;
}

void VRCarDynamics::addWheel(VRGeometryPtr geo, Vec3d p, float radius, float width, float maxSteering, bool steered, bool driven) {
    auto wheel = Wheel::create();
    wheel->ID = wheels.size();
    wheel->position = p;
	wheel->isSteered = steered;
	wheel->isDriven = driven;
    wheel->geo = geo;
    wheel->geo->setPersistency(0);
    wheel->width = width;
    wheel->radius = radius;
    wheel->maxSteer = maxSteering;

    addChild(wheel->geo);
    wheels.push_back(wheel);
    addBTWheel(wheel);
}

void VRCarDynamics::setupSimpleWheels(VRTransformPtr geo, float x, float fZ, float rZ, float h, float r, float w, float ms) {
    // create four simple wheels
    addWheel(static_pointer_cast<VRGeometry>( geo->duplicate() ), Vec3d( x, h, fZ), r, w, ms, true, true);
    addWheel(static_pointer_cast<VRGeometry>( geo->duplicate() ), Vec3d(-x, h, fZ), r, w, ms, true, true);
    addWheel(static_pointer_cast<VRGeometry>( geo->duplicate() ), Vec3d( x, h, rZ), r, w);
    addWheel(static_pointer_cast<VRGeometry>( geo->duplicate() ), Vec3d(-x, h, rZ), r, w);
}

float VRCarDynamics::clamp(float v, float m1, float m2) {
    v = max(m1,v);
    v = min(m2,v);
    return v;
}

float VRCarDynamics::rescale(float v, float m1, float m2) {
    return ( clamp(v,m1,m2)-m1 ) / (m2-m1);
}

void VRCarDynamics::setType(TYPE t) { type = t; }

void VRCarDynamics::updateWheel( WheelPtr wheel, float eForce, float eBreak ) {
    if (wheel->isDriven) {
        vehicle->setBrake(eBreak, wheel->ID);
        vehicle->applyEngineForce(eForce, wheel->ID);
    }

    if (wheel->isSteered) {
        vehicle->setSteeringValue(wheel->steering*wheel->maxSteer, wheel->ID);
    }
}

float VRCarDynamics::computeCoupling( WheelPtr wheel ) {
    if (type == SIMPLE) return (wheel->gear != 0);
    float clutchTransmission = 1;
    if (engine->clutchTransmissionCurve) clutchTransmission = engine->clutchTransmissionCurve->getPosition(wheel->clutch)[1];
    return (wheel->gear != 0)*clutchTransmission;
}

//---------------------------------------------------------------------------------------------------
/*
float VRCarDynamics::computeClutch(){
    float clutchTransmission = 1;
    if (engine->clutchTransmissionCurve) clutchTransmission = engine->clutchTransmissionCurve->getPosition(wheel->clutch)[1];
    return clutchTransmission;
}

float VRCarDynamics::computeThrottle(){
    float clampedThrottle = rescale(wheel->throttle, 0.1, 0.9);
    return clampedTrottle;
}

void VRCarDynamics::computeEngineForce(float clampedThrottle){
    engine->throttledForce = clampedThrottle * engine->maxForce;
    engine->outputForce = 5; //need to model forces hindering engine
}

void VRCarDynamics::computeImpactOnRPM(){
    //falls kupplung drin, kraft negativ auf RPM
    if (engine->rpm < 800) {}
    //muss nachregeln
    //throttle miteinbeziehen
}

void VRCarDynamics::updateEngineRPM(){
    engine->rpm += impactOnRPM;
    if (type != SIMPLE) {
        engine->rpm += 0.1 * deltaRPM;
    }
}

float VRCarDynamics::computeEngineForceOnWheel( WheelPtr wheel, float deltaRPM, float clutchTransmission, float clampedThrottle ) {
    float gearTransmission = engine->gearRatios[wheel->gear];

    return clutchTransmission * engine->outputForce * gearTransmission * engine->running;
}*/
//---------------------------------------------------------------------------------------------------

float VRCarDynamics::computeWheelGearRPM( WheelPtr wheel ) {
    float gearTransmission = engine->gearRatios[wheel->gear];
    float wheelSpeed = abs( getSpeed() ) * 1000/60.0; // from km/h to m/min
    float wheelRPM = wheelSpeed / (wheel->radius * 2 * Pi);
    return wheelRPM * gearTransmission; // meters/min divided by wheel perimeter
}

float VRCarDynamics::computeThrottleTransmission( float clampedThrottle ) {
    /*--TODO--*/
    float throttleTransmissionFkt = 1e-4;
    float torque = engine->torqueCurve->getPosition(engine->rpm)[1]*engine->maxForce;
    //return torque * throttleTransmissionFkt * (engine->maxRpm - engine->rpm) * clampedThrottle * engine->running;
    return torque * throttleTransmissionFkt * (engine->maxRpm - engine->rpm) * clampedThrottle * engine->running;
}

float VRCarDynamics::computeEngineForceOnWheel( WheelPtr wheel, float deltaRPM, float coupling, float clampedThrottle ) {
    float gearTransmission = engine->gearRatios[wheel->gear];
    /*--TODO: CHANGE BEHAVIOUR OF ENGINE THROTTLE--*/
    /* hold engine rpm at minRPM */
    if (type != SIMPLE) {
        float engineF = max( -deltaRPM*0.003f, 0.f); //0.001
        clampedThrottle = clamp(clampedThrottle + engineF, 0, 1); // try to keep the minRPM
    }
    cout << clampedThrottle << endl;
    float torque = engine->torqueCurve->getPosition(engine->rpm)[1]*engine->maxForce;
    return clampedThrottle * torque * coupling * gearTransmission * engine->running;
    //return clampedThrottle * engine->power * coupling * gearTransmission * engine->running;
    //return clampedThrottle * engine->power * coupling * engine->running;
}

float VRCarDynamics::computeEngineFriction( float deltaRPM, float clampedThrottle ) {
    /*--TODO--*/
    float eRPMrange = engine->maxRpm - engine->minRpm;
    float engineFriction = (engine->rpm - engine->minRpm) / eRPMrange * max((deltaRPM*0.003 + 1)*engine->friction, 0.0) * (1.0 - clampedThrottle); //0.001
    if (!engine->running) engineFriction = engine->rpm / engine->minRpm; // engine is not running, blocks everything
    return engineFriction;
}

void VRCarDynamics::updateEngineRPM( float deltaRPM, float throttleImpactOnRPM, float engineFriction ) {
    engine->rpm += throttleImpactOnRPM;
    engine->rpm -= engine->frictionCoefficient * engineFriction;
    if (type != SIMPLE) {
        engine->rpm += 0.1 * deltaRPM;
    }
}

void VRCarDynamics::updateEngine() {
    if (!vehicle) return;
    if (!wheels.size()) return;
    PLock lock(mtx());

    for (uint i=0; i<wheels.size(); i++) {
        auto wheel = wheels[i];
        float coupling = computeCoupling(wheel); // 0 -> 1
        float clampedThrottle = rescale(wheel->throttle, 0.1, 0.9); // stretch throttle range
        float gearRPM = computeWheelGearRPM(wheel);
        float throttleImpactOnRPM = computeThrottleTransmission( clampedThrottle );
        if (abs(gearRPM) > engine->maxRpm) coupling = 0;
        //engine->power = engine->rpm/6; //testing

        // compute breaking
        float deltaRPM = ( abs(gearRPM) - engine->rpm ) * coupling;
        float engineFriction = computeEngineFriction( deltaRPM, clampedThrottle );
        float eBreak = wheel->breaking*engine->breakPower + max(engineFriction, 0.f);
        //engineFriction = 80;
        updateEngineRPM(deltaRPM, throttleImpactOnRPM, engineFriction);
        if (engine->rpm < engine->stallRpm) setIgnition(false);

        float eForce = computeEngineForceOnWheel( wheel, deltaRPM, coupling, clampedThrottle );
        /*
        if (abs(eBreak) > abs(eForce)) eForce = 0;
        else eBreak = 0; *//*
        if (abs(eForce) < 0.1 && abs(coupling) < 0.1) eBreak = max(eBreak, 5.f); // rolling friction when stopped
        */
        if (eBreak <= 5) eBreak = 5;
        //cout << eForce << " - " << eBreak << endl;
        eForces = eForce;
        eBreaks = eBreak;
        updateWheel(wheel, eForce, eBreak);// apply force
    }

    updateSpeedAndAcceleration();
    carSound->play(engine->rpm);
}

void VRCarDynamics::updateSpeedAndAcceleration() {
    speed = vehicle->getCurrentSpeedKmHour();
    double time = glutGet(GLUT_ELAPSED_TIME)*0.001;
    double dt = time-a_measurement_t;
    if (dt > 0) {
        float a = (speed-s_measurement)/dt;
        s_measurement = speed;
        a_measurement_t = time;
        acceleration = a;//abs(a);
    }
}

void VRCarDynamics::setIgnition(bool b) {
    if (type == SIMPLE || type == AUTOMATIC) {
        engine->running = true;
    } else {
        engine->running = b;
        engine->rpm = b ? engine->minRpm : 0;
    }
}

bool VRCarDynamics::isRunning() { return engine->running; }
float VRCarDynamics::getClutch() { return wheels.size() > 0 ? wheels[0]->clutch : 0; }
float VRCarDynamics::getThrottle() { return wheels.size() > 0 ? wheels[0]->throttle : 0; }
float VRCarDynamics::getBreaking() { return wheels.size() > 0 ? wheels[0]->breaking : 0; }
float VRCarDynamics::getSteering() { return wheels.size() > 0 ? wheels[0]->steering : 0; }
int VRCarDynamics::getGear() { return wheels.size() > 0 ? wheels[0]->gear : 0; }
int VRCarDynamics::getRPM() { return engine->rpm; }
float VRCarDynamics::geteForce() { return eForces; }
float VRCarDynamics::geteBreak() { return eBreaks; }

void VRCarDynamics::update(float t, float b, float s, float c, int g) {
    for (uint i=0; i<wheels.size(); i++) updateWheel(i, t, b, s, c, g);
}

void VRCarDynamics::updateWheel(int w, float t, float b, float s, float c, int g) {
    if (w < 0 || w >= wheels.size()) return;
    auto wheel = wheels[w];
    if (!wheel) return;
    wheel->throttle = clamp(t, 0, 1);
    wheel->breaking = clamp(b, 0, 1);
    wheel->clutch = clamp(c, 0, 1);
    wheel->steering = clamp(s, -1, 1);
    wheel->gear = g;
}

void VRCarDynamics::setParameter(float mass, float enginePower, float breakPower, Vec3d massOffset) {
    if (mass > 0) chassis->mass = mass;
    engine->power = enginePower;
    engine->breakPower = breakPower;

    //TODO: pass it
    if (!engine->clutchTransmissionCurve) engine->clutchTransmissionCurve = path::create();
    engine->clutchTransmissionCurve->clear();
    engine->clutchTransmissionCurve->addPoint( Pose(Vec3d(0,1,0), Vec3d(1,0,0)));
    engine->clutchTransmissionCurve->addPoint( Pose(Vec3d(1,0,0), Vec3d(1,0,0)));/*
    engine->clutchTransmissionCurve->addPoint( Pose(Vec3d(0,1,0), Vec3d(1,-1,0)));
    engine->clutchTransmissionCurve->addPoint( Pose(Vec3d(1,0,0), Vec3d(1,-1,0)));*/
    engine->clutchTransmissionCurve->compute(32);

	engine->gearRatios.clear();
	engine->gearRatios[-1] = -3.5;
	engine->gearRatios[0] = 0;
	engine->gearRatios[1] = 3.5;
	engine->gearRatios[2] = 1.5;
	engine->gearRatios[3] = 0.95;
	engine->gearRatios[4] = 0.75;
	engine->gearRatios[5] = 0.63;
	engine->gearRatios[6] = 0.5;
	for (int i=-1; i<=6; i++) engine->gearRatios[i] *= 3.5*1.6;
	engine->minRpm = 800;
	engine->maxRpm = 6000;

    float maxTorqueRPM = engine->minRpm+(engine->maxRpm-engine->minRpm)*0.18;
    if (!engine->torqueCurve) engine->torqueCurve = path::create();
    engine->torqueCurve->clear();
    engine->torqueCurve->addPoint( Pose(Vec3d(engine->stallRpm,0.5,0), Vec3d(0.5,1,0)));
    engine->torqueCurve->addPoint( Pose(Vec3d(engine->minRpm,0.75,0), Vec3d(0.5,1,0)));
    engine->torqueCurve->addPoint( Pose(Vec3d(engine->maxTorqueRPM,1,0), Vec3d(1,0,0)));
    engine->torqueCurve->addPoint( Pose(Vec3d(engine->maxRpm,0.5,0), Vec3d(1,-0.5,0)));
    engine->torqueCurve->compute(32);

	// update physics
	if (!chassis->geo) return;
    PLock lock(mtx());
    for ( auto geo : chassis->geos ) {
        geo->setMatrix(Matrix4d());
        auto p = geo->getPoseTo(chassis->geo);
        geo->setFrom( p->pos() - massOffset + chassis->massOffset );
        geo->applyTransformation();
    }
    chassis->massOffset = massOffset;
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

void VRCarDynamics::reset(const Pose& p) {
    PLock lock(mtx());
    setIgnition(false);
	btTransform t;
	t.setIdentity();
	auto m = toMatrix4f( p.asMatrix() );
	t.setFromOpenGLMatrix(&m[0][0]);
	chassis->body->setCenterOfMassTransform(t);
	chassis->body->setLinearVelocity(btVector3(0, 0, 0));
	chassis->body->setAngularVelocity(btVector3(0, 0, 0));
	m_dynamicsWorld->getBroadphase()->getOverlappingPairCache()->cleanProxyFromPairs(chassis->body->getBroadphaseHandle(), m_dynamicsWorld->getDispatcher());
	if (vehicle) {
		vehicle->resetSuspension();
		for (int i = 0; i<vehicle->getNumWheels(); i++) {
			//synchronize the wheels with the (interpolated) chassis worldtransform
			vehicle->updateWheelTransform(i, true);
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



