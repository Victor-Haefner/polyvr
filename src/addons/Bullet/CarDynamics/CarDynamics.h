#ifndef CARDYNAMICS_H_INCLUDED
#define CARDYNAMICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <btBulletDynamicsCommon.h>
#include <LinearMath/btVector3.h>
#include <boost/thread/recursive_mutex.hpp>
#include "core/utils/VRFunctionFwd.h"
#include "core/tools/VRToolsFwd.h"
#include "core/objects/object/VRObject.h"

class pose;

OSG_BEGIN_NAMESPACE;
using namespace std;

class CarDynamics : public VRObject {
    public:
        struct Wheel {
            VRGeometryPtr geo;

            // suspension parameter
            Vec3f position;
            Vec3f direction = Vec3f(0, -1, 0);
            Vec3f axle = Vec3f(-1, 0, 0);
            float suspensionRestLength = 0.6;
            float suspensionStiffness = 20.f;
            float suspensionDamping = 2.3f;
            float suspensionCompression = 4.4f;
            float rollInfluence = 0.1f;//1.0f;
            bool isFrontWheel = false;

            // wheel parameter
            float friction = 1000;//BT_LARGE_FLOAT;
            float radius = .4f;
            float width = 0.4f;
        };

        struct Engine {
            // engine parameter
            float power = 1000;//this should be engine/velocity dependent
            float breakPower = 70;//this should be engine/velocity dependent
            float maxForce = 10000;//this should be engine/velocity dependent
            float maxBreakingForce = 100;
            float maxSteer = .3f;
        };

        struct Chassis {
            VRGeometryPtr geo;
            btRigidBody* body = 0;
            float mass = 850.0f;
        };

    private:
        // user inputs
        float throttle = 0;
        float breaking = 0;
        float steering = 0;

        vector<Wheel> wheels;
        Engine engine;
        Chassis chassis;

        VRUpdateCbPtr updateWPtr;
        VRUpdateCbPtr updatePPtr;

        btAlignedObjectArray<btCollisionShape*> m_collisionShapes;
        btRaycastVehicle::btVehicleTuning m_tuning;
        btVehicleRaycaster*	m_vehicleRayCaster = 0;
        btRaycastVehicle* m_vehicle = 0;
        btDynamicsWorld* m_dynamicsWorld = 0;

        // simplified parameters for wheels setup
        float xOffset = 1.78f;
        float frontZOffset = 2.9f;
        float rearZOffset = -2.7f;
        float height = .4f;

        // pilot
        bool doPilot = false;
        pathPtr p_path;
        pathPtr v_path;

        boost::recursive_mutex& mtx();

        void initPhysics();
        void initVehicle();

        void updatePilot();
        void updateWheels();

        btRigidBody* createRigitBody(float mass, const btTransform& startTransform, btCollisionShape* shape);

    public:
        CarDynamics(string name);
        ~CarDynamics();
        static CarDynamicsPtr create(string name);

        VRObjectPtr getRoot();
        VRTransformPtr getChassis();
        vector<VRTransformPtr> getWheels();

        void setThrottle(float t);
        void setBreak(float b);
        void setSteering(float s);
        float getThrottle();
        float getBreaking();
        float getSteering();

        void setChassisGeo(VRGeometryPtr geo, bool doPhys = 1);
        void setupSimpleWheels(VRGeometryPtr geo, float xOffset, float frontZOffset, float rearZOffset, float height, float radius, float width);
        void setParameter(float mass, float maxSteering, float enginePower, float breakPower);

        void followPath(pathPtr p, pathPtr v);
        void stopPilot();
        bool onAutoPilot();

        void reset(const pose& p);
        float getSpeed();
        float getAcceleration();
};

OSG_END_NAMESPACE

#endif // CARDYNAMICS_H_INCLUDED
