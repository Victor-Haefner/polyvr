#include "CarDynamics.h"

#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTextureObjChunk.h>

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/object/VRObjectT.h"
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>

#include <stdio.h> //printf debugging

typedef boost::recursive_mutex::scoped_lock PLock;

OSG_BEGIN_NAMESPACE;
using namespace std;

int rightIndex = 0;
int upIndex = 1;
int forwardIndex = 2;
btVector3 wheelDirectionCS0(0, -1, 0);
btVector3 wheelAxleCS(-1, 0, 0);
btScalar suspensionRestLength(0.6);

//
//
//const int maxProxies = 32766;
//const int maxOverlap = 65535;
//
//float	gEngineForce = 0.f;
//float	gBreakingForce = 0.f;
//
float	maxEngineForce = 10000.f;//this should be engine/velocity dependent
float	maxBreakingForce = 100.f;
//
float	gVehicleSteering = 0.f;
//float	steeringIncrement = 0.04f;
//float	steeringClamp = 0.3f;

//for vehicle tuning
float	wheelFriction = 1000;//BT_LARGE_FLOAT;
float	suspensionStiffness = 20.f;
float	suspensionDamping = 2.3f;
float	suspensionCompression = 4.4f;
float	rollInfluence = 0.1f;//1.0f;
float m_mass = 850.0f;

//params for the setting the wheels && axis
float xOffset = 1.78f;
float frontZOffset = 2.9f;
float rearZOffset = -2.7f;
float height = .4f;
float	wheelRadius = .4f;
float	wheelWidth = 0.4f;
//
//
//bool printedWheelTrans = false;
bool initialBuilt = false;
//

#define CUBE_HALF_EXTENTS 1
CarDynamics::CarDynamics() {
    root = new VRObject("car");
    root->setPersistency(0);
    m_defaultContactProcessingThreshold = BT_LARGE_FLOAT;
	w1 = 0;
	w2 = 0;
	w3 = 0;
	w4 = 0;

	initPhysics();
}

//only to be done once
void CarDynamics::initPhysics(){
    VRScene* scene = VRSceneManager::getCurrent();
    VRFunction<int>* fkt = new VRFunction<int>("cardyn_update", boost::bind(&CarDynamics::updateWheels, this));
    scene->addUpdateFkt(fkt);
    m_dynamicsWorld = (btDynamicsWorld*) scene->bltWorld();
}

//only to be done once
float CarDynamics::getSpeed() { PLock lock(mtx()); return m_vehicle->getCurrentSpeedKmHour(); }

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

void CarDynamics::setChassisGeo(VRGeometry* geo) {
    geo->setMatrix(Matrix());
    geo->getPhysics()->setShape("Convex");
    geo->getPhysics()->setMass(m_mass);
    geo->getPhysics()->setDynamic(true);
    geo->getPhysics()->setPhysicalized(true);
    geo->getPhysics()->updateTransformation(geo);

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
    root->addChild(geo);
}

void CarDynamics::setWheelGeo(VRGeometry* geo) { // TODO
    if (w1) delete w1;
    if (w2) delete w2;
    if (w3) delete w3;
    if (w4) delete w4;

    w1 = (VRGeometry*)geo->duplicate();
    w2 = (VRGeometry*)geo->duplicate();
    w3 = (VRGeometry*)geo->duplicate();
    w4 = (VRGeometry*)geo->duplicate();
    w1->setPersistency(0);
    w2->setPersistency(0);
    w3->setPersistency(0);
    w4->setPersistency(0);

    root->addChild(w1);
    root->addChild(w2);
    root->addChild(w3);
    root->addChild(w4);

    cout << "\nset wheel geo " << geo->getName() << endl;
}

VRObject* CarDynamics::getRoot() { return root; }

void CarDynamics::setWheelOffsets(float x, float fZ, float rZ, float h){
    if(x!=-1) xOffset = x;
    if(fZ!=-1) frontZOffset = fZ;
    if(rZ!=-1) rearZOffset = rZ;
    if(h!=-1) height = h;
}

void CarDynamics::setWheelParams(float w, float r){
    if(w!=-1) wheelWidth = w;
    if(r!=-1) wheelRadius = r;
}

void CarDynamics::setThrottle(float t) {
    //t = max(0.f,t);
    if (t>0) t = min(maxEngineForce,t);
    else     t = max(-maxEngineForce,t);

    PLock lock(mtx());
    m_vehicle->applyEngineForce(t, 2);
    m_vehicle->applyEngineForce(t, 3);

    //cout << "\nset throttle " << t << endl;
}

void CarDynamics::setBreak(float b) {
    b = max(0.f,b);
    b = min(maxBreakingForce,b);

    PLock lock(mtx());
    m_vehicle->setBrake(b, 2);
    m_vehicle->setBrake(b, 3);

}

void CarDynamics::setSteering(float s) {
    float max_steer = .3f;
    PLock lock(mtx());

    if (s < -1) {
        m_vehicle->setSteeringValue(-max_steer, 0);
        m_vehicle->setSteeringValue(-max_steer, 1);
        return;
    } else if (s > 1) {
        m_vehicle->setSteeringValue(max_steer, 0);
        m_vehicle->setSteeringValue(max_steer, 1);
        return;
    }

    float res = (((s+1)*(2*max_steer)) / (2)) - max_steer;

    m_vehicle->setSteeringValue(res, 0);
    m_vehicle->setSteeringValue(res, 1);
}

void CarDynamics::setCarMass(float m) {
    if(m > 0) m_mass = m;
}

boost::recursive_mutex& CarDynamics::mtx() {
    auto scene = OSG::VRSceneManager::getCurrent();
    if (scene) return scene->physicsMutex();
    else {
        static boost::recursive_mutex m;
        return m;
    };
}

void CarDynamics::reset(float x, float y, float z) {
    PLock lock(mtx());

	gVehicleSteering = 0.f;
	btTransform t;
	t.setIdentity();
	t.setOrigin(btVector3(x,y,z));
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
//	btRigidBody* body = new btRigidBody(mass, 0, shape, localInertia);
//	body->setWorldTransform(startTransform);

	//dynamicsWorld->addRigidBody(body);

	return body;
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

OSG_END_NAMESPACE
