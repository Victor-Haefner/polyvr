#ifndef CARDYNAMICS_H_INCLUDED
#define CARDYNAMICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <btBulletDynamicsCommon.h>
#include <boost/thread/recursive_mutex.hpp>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObject;
class VRGeometry;

class CarDynamics {
    private:
        VRGeometry *w1, *w2, *w3, *w4;
        VRGeometry* chassis = 0;
        VRObject* root = 0;

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
        CarDynamics();
        ~CarDynamics();

        VRObject* getRoot();

        void setThrottle(float t);
        void setBreak(float b);
        void setSteering(float s);

        void setChassisGeo(VRGeometry* geo);
        void setWheelGeo(VRGeometry* geo);
        void setWheelOffsets(float xOffset, float frontZOffset, float rearZOffset, float height);
        void setWheelParams(float w, float r);
        void setCarMass(float m);

        void updateWheels();

        void reset(float x, float y, float z);
        float getSpeed();
};

OSG_END_NAMESPACE

#endif // CARDYNAMICS_H_INCLUDED
