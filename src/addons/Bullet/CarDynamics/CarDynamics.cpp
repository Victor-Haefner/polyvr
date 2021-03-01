#include "CarDynamics.h"
#ifndef WITHOUT_AV
#include "CarSound/CarSound.h"
#endif
#include "core/scene/VRScene.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/object/VRObjectT.h"
#include "core/math/pose.h"
#include "core/math/path.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/VRStorage_template.h"

#include <boost/thread/recursive_mutex.hpp>
#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <math.h>

typedef boost::recursive_mutex::scoped_lock PLock;

using namespace OSG;

VRCarDynamics::Wheel::Wheel() {
    position = new Vec3d();
    direction = new Vec3d(0, -1, 0);
    axle = new Vec3d(-1, 0, 0);

    // suspension parameter
    store("position", position);
    store("direction", direction);
    store("axle", axle);
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

VRCarDynamics::Wheel::~Wheel() {
    delete position;
    delete direction;
    delete axle;
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
    massOffset = new Vec3d();

    store("mass", &mass);
    store("massOffset", massOffset);
}

VRCarDynamics::Chassis::~Chassis() {
    delete massOffset;
}

VRCarDynamics::WheelPtr VRCarDynamics::Wheel::create() { return WheelPtr( new Wheel() ); }
VRCarDynamics::ChassisPtr VRCarDynamics::Chassis::create() { return ChassisPtr( new Chassis() ); }
VRCarDynamics::EnginePtr VRCarDynamics::Engine::create() { return EnginePtr( new Engine() ); }

VRCarDynamics::VRCarDynamics(string name) : VRObject(name) {
    engine = Engine::create();
    chassis = Chassis::create();

    setPersistency(0);
	initPhysics();
#ifndef WITHOUT_AV
	carSound = VRCarSound::create();
#endif

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

VRCarSoundPtr VRCarDynamics::getCarSound() { return carSound; }

//only to be done once
void VRCarDynamics::initPhysics() {
    auto scene = VRScene::getCurrent();
    updateEPtr = VRUpdateCb::create("cardyn_engin_update", bind(&VRCarDynamics::updateEngine, this));
    updateWPtr = VRUpdateCb::create("cardyn_wheel_update", bind(&VRCarDynamics::updateWheelGeos, this));
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
    chassis->geo->getPhysics()->setCenterOfMass(*chassis->massOffset);
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
    btVector3 pos = VRPhysics::toBtVector3(*wheel->position - *chassis->massOffset);
    btVector3 dir = VRPhysics::toBtVector3(*wheel->direction);
    btVector3 axl = VRPhysics::toBtVector3(*wheel->axle);
    btWheelInfo& btWheel = vehicle->addWheel(pos, dir, axl, wheel->suspensionRestLength, wheel->radius, m_tuning, wheel->isSteered);
    btWheel.m_suspensionStiffness = wheel->suspensionStiffness;
    btWheel.m_wheelsDampingRelaxation = wheel->suspensionDamping;
    btWheel.m_wheelsDampingCompression = wheel->suspensionCompression;
    btWheel.m_frictionSlip = wheel->friction;
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
    *wheel->position = p;
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
    addWheel(static_pointer_cast<VRGeometry>( geo->duplicate() ), Vec3d( x, h, fZ), r, w, ms, true, true); // front steered + back driven
    addWheel(static_pointer_cast<VRGeometry>( geo->duplicate() ), Vec3d(-x, h, fZ), r, w, ms, true, true); // front steered + back driven
    addWheel(static_pointer_cast<VRGeometry>( geo->duplicate() ), Vec3d( x, h, rZ), r, w, 0, false, false);
    addWheel(static_pointer_cast<VRGeometry>( geo->duplicate() ), Vec3d(-x, h, rZ), r, w, 0, false, false);
}

float VRCarDynamics::clamp(float v, float m1, float m2) {
    v = max(m1,v);
    v = min(m2,v);
    return v;
}

float VRCarDynamics::rescale(float v, float m1, float m2) {
    return ( clamp(v,m1,m2)-m1 ) / (m2-m1);
}

float VRCarDynamics::strech(float v, float m1) {
    return v*(1-m1)+m1; //input 0->1, output m1->1
}

void VRCarDynamics::setType(TYPE t) { type = t; }

void VRCarDynamics::updateWheelForces( WheelPtr wheel, float eForce, float eBreak ) {
    vehicle->setBrake(eBreak, wheel->ID);

    if (wheel->isDriven) vehicle->applyEngineForce(eForce, wheel->ID);
    if (wheel->isSteered) vehicle->setSteeringValue(wheel->steering*wheel->maxSteer, wheel->ID);
}

//--------------------------------------------------------------------------------------------------------------------------------

float VRCarDynamics::computeCoupling( WheelPtr wheel ) {
    if (type == SIMPLE) return (wheel->gear != 0);
    float clutchTransmission = 1;
    if (engine->clutchTransmissionCurve) clutchTransmission = engine->clutchTransmissionCurve->getPosition(wheel->clutch)[1];
    return (wheel->gear != 0)*clutchTransmission;
}

//--if engine needs more power and rpm drop below minRpm, boosts throttle slightly, can be ajusted to make clutch more easy/hard
float VRCarDynamics::throttleBooster( float clampedThrottle ) {
    float toPow = ( 1 - ((engine->rpm-50 - engine->stallRpm)/(engine->minRpm - engine->stallRpm)) ); //non linear response
    if ( engine->rpm < (engine->minRpm-5) && clampedThrottle<=0.3) {
        float ccThrottle = engine->minThrottle + toPow*toPow;
        if ( engine->rpm < engine->minRpm && ccThrottle < 0.09) return 0.09; //minThrottle under load
        else return ccThrottle;
    }
    else return clampedThrottle;
}

float VRCarDynamics::computeThrottle( float pedalPos ) {
    float retThrottle = 0;
    if (type != SIMPLE) retThrottle = throttleBooster(retThrottle);
    retThrottle = strech(rescale(pedalPos, 0.05, 0.95)*rescale(pedalPos, 0.05, 0.95), retThrottle); //shifts pedalPos above minThrottle
    if (engine->rpm > engine->maxRpm) retThrottle = 0;
    return retThrottle;
}

float VRCarDynamics::computeWheelGearRPM( WheelPtr wheel ) {
    float gearTransmission = engine->gearRatios[wheel->gear];
    float wheelSpeed = abs( getSpeed() ) * 1000/60.0; // from km/h to m/min
    float wheelRPM = wheelSpeed / (wheel->radius * 2 * Pi);
    return wheelRPM * gearTransmission; // meters/min divided by wheel perimeter
}

float VRCarDynamics::computeThrottleTransmission( float clampedThrottle ) {
    float throttleTransmissionFkt = 1e-4;
    float torque = engine->torqueCurve->getPosition(engine->rpm)[1]*engine->maxForce;
    return torque * throttleTransmissionFkt * 5000 * clampedThrottle * engine->running; //5000 used to be engine->maxRpm - engine->rpm
}

float VRCarDynamics::computeBreakTransmission( WheelPtr wheel, float coupling, float clampedThrottle ) {
    float a = 11.5741; //[m/s²] max breaking deceleration
    double time = getTime()*1e-6;
    double dt = time-a_measurement_t;
    float aRPM = a * 60 / (wheel->radius * 2 * Pi);
    float breakImpact = wheel->breaking * aRPM * dt * coupling; //parameters to stop engine if breaks are being used
    breakImpact = 0;
    return breakImpact; //Impact of break-forces on engineRPM
}

float VRCarDynamics::computeEngineForceOnWheel( WheelPtr wheel, float gearRPM, float deltaRPM, float coupling, float clampedThrottle ) {
    float gearTransmission = engine->gearRatios[wheel->gear];
    /*
    if (type != SIMPLE) {
        float engineF = max( -deltaRPM*0.003f, 0.f);
        if (gearRPM < 0) engineF = 0;
        clampedThrottle = clamp(clampedThrottle + engineF, 0, 1); // try to keep the minRPM
    }*/
    float torque = engine->torqueCurve->getPosition(engine->rpm)[1]*engine->maxForce;
    return clampedThrottle * torque * coupling * gearTransmission * engine->running / wheel->radius /2;
}

float VRCarDynamics::computeAirResistence( float vehicleVelocity ) {
    vehicleVelocity = abs(vehicleVelocity);
    float airResistance = chassis->cw*chassis->airA*rhoAir*vehicleVelocity*vehicleVelocity/2; //cw*A*rho*v^2/2
    return airResistance; //asdf
}

float VRCarDynamics::computeEngineFriction( float gear, float deltaRPM, float coupling, float clampedThrottle ) {
    float eRPMrange = engine->maxRpm - engine->minRpm;
    float engineFriction = (engine->rpm - engine->minRpm) / eRPMrange * max((deltaRPM*0.003 + 1)*engine->friction, 0.0) * (1.0 - clampedThrottle);
    if (!engine->running) engineFriction = engine->rpm / engine->minRpm; // engine is not running, blocks everything

    engineFriction += engine->rpm / engine->maxRpm * 2.3; //if (engine->rpm<800 && engine->running)
    //if (coupling>0.7 && clampedThrottle<0.6) engineFriction += engine->rpm / engine->maxRpm * 2.6;
    return engineFriction;
}

float VRCarDynamics::computeEngineBreak(float gearRatio, float coupling ) {
    if (abs(gearRatio)>10) return 5 * coupling * 100;
    if (abs(gearRatio)<10) return coupling * 100;
    return 0;
    return abs(gearRatio) * coupling *100; //abs(gearRatio) * coupling * engine->rpm / engine->maxRpm * 200;
}

void VRCarDynamics::updateEngineRPM( float gearRPM, float deltaRPM, float throttleImpactOnRPM, float breakImpactOnRPM, float engineFriction, float coupling ) {
    float slidingFactor = 0.0;
    for (uint each = 0; each < wheels.size(); each ++) {
        btWheelInfo& wheelInf =  vehicle->getWheelInfo(each);
        auto skidInf = float(wheelInf.m_skidInfo);
        slidingFactor += skidInf;
        //cout << " VRCarDynamics::skidInfo " << toString(skidInf) << endl;
    }
    slidingFactor *= 1.0/float(wheels.size());
    //if (slidingFactor < 1) cout << " VRCarDynamics::skidInfo " << slidingFactor << endl;
    if (coupling > 0.98 && slidingFactor>0.80) engine->rpm = abs(gearRPM);
    engine->rpm += throttleImpactOnRPM;
    engine->rpm -= engine->frictionCoefficient * engineFriction + breakImpactOnRPM;
    if (type != SIMPLE) {
        engine->rpm += 0.1 * deltaRPM;
    }
    if (coupling > 0.98 && slidingFactor>0.98) engine->rpm = abs(gearRPM);
}

void VRCarDynamics::updateEngine() {
    if (!vehicle) return;
    if (!wheels.size()) return;

    //VRTimer timer;
    //timer.start("D1");

    PLock lock(mtx());

    for (uint i=0; i<wheels.size(); i++) {
        auto wheel = wheels[i];
        /*
        if(weather.isRaining()==true) {
            wheel->friction = 0.8;
            btWheel.m_frictionSlip = wheel->friction;
        }
        */
        float coupling = computeCoupling(wheel); // 0 -> 1
        float clampedThrottle = computeThrottle(wheel->throttle); //strech(rescale(wheel->throttle, 0.05, 0.95),engine->minThrottle); // stretch throttle range
        float gearRPM = computeWheelGearRPM(wheel);
        float throttleImpactOnRPM = computeThrottleTransmission( clampedThrottle );
        float breakImpactOnRPM = computeBreakTransmission( wheel, coupling, clampedThrottle );
        if (abs(gearRPM) > engine->maxRpm) coupling = 0;

        // compute breaking
        float torque = engine->torqueCurve->getPosition(engine->rpm)[1]*engine->maxForce;
        engine->power = engine->rpm*torque;
        float deltaRPM = 0;
        float lhs = vehicle->getCurrentSpeedKmHour();
        float rhs = engine->gearRatios[wheel->gear];
        if ((lhs >= 0 && rhs >=0) || (lhs<0 && rhs<0)) deltaRPM = ( abs(gearRPM) - engine->rpm ) * coupling;    //deltaRPM for rolling forwards + positive gear, or rolling backwards + reverse gear
        if ((lhs >= 0 && rhs <=0) || (lhs<0 && rhs>0)) deltaRPM = ( -abs(gearRPM) - engine->rpm ) * coupling;   //deltaRPM for rolling rolling forwards + reverse gear, or backwards + positive gear
        float gear = rhs;

        float engineFriction = computeEngineFriction( gear, deltaRPM, coupling, clampedThrottle );
        float eBreak = 0; //wheel->breaking*engine->breakPower + computeEngineBreak( coupling, clampedThrottle );
        float wBreak = engine->breakCurve->getPosition(wheel->breaking)[1]*engine->maxBreakingForce;
        updateEngineRPM(gearRPM, deltaRPM, throttleImpactOnRPM, breakImpactOnRPM, engineFriction, coupling); //only needed during clutch

        if (engine->rpm < engine->stallRpm) setIgnition(false);
        //if (engine->rpm > engine->maxRpm*1.2) setIgnition(false);
        float airResistancePerWheel = computeAirResistence(vehicle->getCurrentSpeedKmHour()) / wheels.size()*wheel->radius;

        float forcePart = computeEngineForceOnWheel( wheel, gearRPM, deltaRPM, coupling, clampedThrottle );
        float breakPart = airResistancePerWheel + computeEngineBreak(engine->gearRatios[wheel->gear], coupling)/wheels.size();
        if (forcePart<0) breakPart = -breakPart;
        float eForce = forcePart - breakPart;
        if (debugCarDyn) cout << "clThrottle: " << clampedThrottle << " gearR " << engine->gearRatios[wheel->gear] << endl;
        eBreak += wBreak;
        if (vehicle->getCurrentSpeedKmHour() < 50 && eForce < 700) eBreak += 1.8;
        if (abs(breakPart)>abs(forcePart)) {eBreak += abs(eForce)/100 + 2.8; eForce = 0;}

        if (wBreak > 50) eForce = 0;
        if (eForce > 0) eBreak = 2*eBreak; //compensate BulletBehaviour 2xBrakes missing if engineForce being applied

        if (debugCarDyn) cout << "forcePart: " << forcePart << " breakPart: " << breakPart << endl;
        if (debugCarDyn) cout << "eForce: " << eForce << " eBreak: " << eBreak << endl;
        eForces = eForce;
        eBreaks = eBreak;

        updateWheelForces(wheel, eForce, eBreak);// apply forces
    }

    updateSpeedAndAcceleration();
#ifndef WITHOUT_AV
    carSound->play(engine->rpm);
#endif

	//auto D1 = timer.stop("D1");
	//if (D1 > 3) cout << "   TTT " << D1 << endl;
}

void VRCarDynamics::updateSpeedAndAcceleration() {
    speed = vehicle->getCurrentSpeedKmHour();
    double time = getTime()*1e-6;
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
        //engine->rpm = b ? engine->minRpm : 0;
        if (b) engine->rpm = b ? engine->minRpm : 0;
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
    if (w < 0 || w >= (int)wheels.size()) return;
    auto wheel = wheels[w];
    if (!wheel) return;
    wheel->throttle = clamp(t, 0, 1);
    wheel->breaking = clamp(b, 0, 1);
    wheel->clutch = clamp(c, 0, 1);
    wheel->steering = clamp(s, -1, 1);
    wheel->gear = g;
}

void VRCarDynamics::setParameter(float mass, float enginePower, float breakPower, const Vec3d& massOffset) {
    if (mass > 0) chassis->mass = mass;
    engine->power = enginePower;
    engine->breakPower = breakPower;

    //TODO: pass it
    if (!engine->clutchTransmissionCurve) engine->clutchTransmissionCurve = Path::create();
    engine->clutchTransmissionCurve->clear();
    engine->clutchTransmissionCurve->addPoint( Pose(Vec3d(0,1,0), Vec3d(1,0,0)));
    engine->clutchTransmissionCurve->addPoint( Pose(Vec3d(1,0,0), Vec3d(1,0,0)));
    engine->clutchTransmissionCurve->compute(32);

	engine->gearRatios.clear();
	engine->gearRatios[-1] = -3.5;
	engine->gearRatios[0] = 0;
	engine->gearRatios[1] = 3.5;
	engine->gearRatios[2] = 2.3; //1.5;
	engine->gearRatios[3] = 1.8; //0.95;
	engine->gearRatios[4] = 1.2; //0.75;
	engine->gearRatios[5] = 1.0; //0.63;
	engine->gearRatios[6] = 0.8; //0.5;
	for (int i=-1; i<=6; i++) engine->gearRatios[i] *= 4.0 *0.8;
	engine->minRpm = 800;
	engine->maxRpm = 6000;

    //float maxTorqueRPM = engine->minRpm+(engine->maxRpm-engine->minRpm)*0.70;
    if (!engine->torqueCurve) engine->torqueCurve = Path::create();
    engine->torqueCurve->clear();
    engine->torqueCurve->addPoint( Pose(Vec3d(engine->stallRpm,0.8,0), Vec3d(1,0,0)));
    engine->torqueCurve->addPoint( Pose(Vec3d(engine->minRpm,0.92,0), Vec3d(1,0,0)));
    engine->torqueCurve->addPoint( Pose(Vec3d(engine->maxTorqueRPM,1,0), Vec3d(1,0,0)));
    engine->torqueCurve->addPoint( Pose(Vec3d(engine->maxRpm,0.8,0), Vec3d(1,0,0)));
    engine->torqueCurve->compute(32);

    if (!engine->breakCurve) engine->breakCurve = Path::create();
    engine->breakCurve->clear();
    engine->breakCurve->addPoint( Pose(Vec3d(0,0,0), Vec3d(1,0,0)));
    engine->breakCurve->addPoint( Pose(Vec3d(0.6,0.3,0), Vec3d(1,1,0)));
    engine->breakCurve->addPoint( Pose(Vec3d(1,1,0), Vec3d(0,1,0)));
    engine->breakCurve->compute(32);

	// update physics
	if (!chassis->geo) return;
    PLock lock(mtx());
    for ( auto geo : chassis->geos ) {
        geo->setMatrix(Matrix4d());
        auto p = chassis->geo->getPoseTo(geo);
        geo->setFrom( p->pos() - massOffset + *chassis->massOffset );
        geo->applyTransformation();
    }
    *chassis->massOffset = massOffset;
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



