#ifndef CARDYNAMICS_H_INCLUDED
#define CARDYNAMICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <btBulletDynamicsCommon.h>
#include <LinearMath/btVector3.h>
#include <boost/thread/recursive_mutex.hpp>
#include "core/utils/VRFunctionFwd.h"
#include "core/tools/VRToolsFwd.h"
#include "core/objects/object/VRObject.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class CarDynamics : public VRObject {
    private:
        int rightIndex = 0;
        int upIndex = 1;
        int forwardIndex = 2;
        btVector3 wheelDirectionCS0 = btVector3(0, -1, 0);
        btVector3 wheelAxleCS = btVector3(-1, 0, 0);
        btScalar suspensionRestLength = 0.6;

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


        VRGeometryPtr w1, w2, w3, w4;
        VRGeometryPtr chassis = 0;
        VRUpdatePtr updatePtr;

        btAlignedObjectArray<btCollisionShape*> m_collisionShapes;

        btRigidBody* m_carChassis = 0;
        btRaycastVehicle::btVehicleTuning	m_tuning;
        btVehicleRaycaster*	m_vehicleRayCaster = 0;
        btRaycastVehicle*	m_vehicle = 0;
        btDynamicsWorld* m_dynamicsWorld = 0;

        btScalar m_defaultContactProcessingThreshold;

        boost::recursive_mutex& mtx();

        void initPhysics();
        void initVehicle();

        btRigidBody* createRigitBody(float mass, const btTransform& startTransform, btCollisionShape* shape);

    public:
        CarDynamics(string name);
        ~CarDynamics();

        static CarDynamicsPtr create(string name);

        VRObjectPtr getRoot();

        void setThrottle(float t);
        void setBreak(float b);
        void setSteering(float s);

        void setChassisGeo(VRGeometryPtr geo, bool doPhys = 1);
        void setWheelGeo(VRGeometryPtr geo);
        void setWheelOffsets(float xOffset, float frontZOffset, float rearZOffset, float height);
        void setWheelParams(float w, float r);
        void setCarMass(float m);

        void updateWheels();

        void reset(float x, float y, float z);
        float getSpeed();
};

OSG_END_NAMESPACE

#endif // CARDYNAMICS_H_INCLUDED
