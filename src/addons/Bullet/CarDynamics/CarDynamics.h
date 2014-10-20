#ifndef CARDYNAMICS_H_INCLUDED
#define CARDYNAMICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <btBulletDynamicsCommon.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGeometry;

class CarDynamics {
    private:
        VRGeometry *w1, *w2, *w3, *w4;
        VRGeometry* chassis;

        btAlignedObjectArray<btCollisionShape*> m_collisionShapes;

        btRigidBody* m_carChassis;
        btRaycastVehicle::btVehicleTuning	m_tuning;
        btVehicleRaycaster*	m_vehicleRayCaster;
        btRaycastVehicle*	m_vehicle;
        btDynamicsWorld* m_dynamicsWorld;

        btScalar m_defaultContactProcessingThreshold;

        void initPhysics();
        void initVehicle();
        void resetVehicle();

        btRigidBody* createRigitBody(float mass, const btTransform& startTransform, btCollisionShape* shape);

    public:
        CarDynamics();
        ~CarDynamics();

        void setThrottle(float t);
        void setBreak(float b);
        void setSteering(float s);

        void setChassisGeo(VRGeometry* geo);
        void setWheelGeo(VRGeometry* geo);
        void setWheelOffsets(float xOffset, float frontZOffset, float rearZOffset, float height);
        void setWheelParams(float w, float r);

        void updateWheels();

        void reset();
        float getSpeed();
};

OSG_END_NAMESPACE

#endif // CARDYNAMICS_H_INCLUDED
