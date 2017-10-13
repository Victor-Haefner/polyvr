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
#include "addons/Bullet/VRPhysicsFwd.h"

class pose;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRCarDynamics : public VRObject {
    public:
        struct Wheel : public VRStorage {
            VRTransformPtr geo;
            int ID = -1;

            // suspension parameter
            Vec3d position;
            Vec3d direction = Vec3d(0, -1, 0);
            Vec3d axle = Vec3d(-1, 0, 0);
            float suspensionRestLength = 0.6;
            float suspensionStiffness = 20.f;
            float suspensionDamping = 2.3f;
            float suspensionCompression = 4.4f;
            float rollInfluence = 0.1f;//1.0f;
            float maxSteer = 0.3;
            bool isSteered = false;
            bool isDriven = false;

            // wheel parameter
            float friction = 1000;//BT_LARGE_FLOAT;
            float radius = .4f;
            float width = 0.4f;

            // user inputs
            int gear = 0;
            float clutch = 0;
            float throttle = 0;
            float breaking = 0;
            float steering = 0;

            Wheel();
            static shared_ptr<Wheel> create();
        };

        struct Engine : public VRStorage {
            // engine parameter
            float power = 1000;//this should be engine/velocity dependent
            float breakPower = 70;//this should be engine/velocity dependent
            float maxForce = 10000;//this should be engine/velocity dependent
            float maxBreakingForce = 100;
            float rpm = 800;
            float minRpm = 800;
            float maxRpm = 4500;
            float stallRpm = 480;
            float friction = 5;
            float frictionCoefficient = 14;
            map<int,float> gearRatios;
            pathPtr clutchTransmissionCurve;
            bool running = false;

            Engine();
            static shared_ptr<Engine> create();
        };

        struct Chassis : public VRStorage {
            VRTransformPtr geo;
            vector<VRGeometryPtr> geos;
            btRigidBody* body = 0;
            float mass = 850.0f;
            Vec3d massOffset;

            Chassis();
            static shared_ptr<Chassis> create();
        };

        enum TYPE {
            SIMPLE = 0,
            AUTOMATIC,
            SEMIAUTOMATIC,
            MANUAL
        };

        typedef shared_ptr<Engine> EnginePtr;
        typedef shared_ptr<Chassis> ChassisPtr;
        typedef shared_ptr<Wheel> WheelPtr;

    private:
        int type = SIMPLE;
        EnginePtr engine;
        ChassisPtr chassis;
        vector<WheelPtr> wheels;

        CarSoundPtr carSound;
        VRUpdateCbPtr updateEPtr;
        VRUpdateCbPtr updateWPtr;

        btAlignedObjectArray<btCollisionShape*> m_collisionShapes;
        btRaycastVehicle::btVehicleTuning m_tuning;
        btVehicleRaycaster*	vehicleRayCaster = 0;
        btRaycastVehicle* vehicle = 0;
        btDynamicsWorld* m_dynamicsWorld = 0;

        // simplified parameters for wheels setup
        float xOffset = 1.78f;
        float frontZOffset = 2.9f;
        float rearZOffset = -2.7f;
        float height = .4f;

        float speed = 0;
        float acceleration = 0;
        float s_measurement = 0;
        double a_measurement_t = 0;

        float clamp(float v, float m1, float m2);
        float rescale(float v, float m1, float m2);

        float computeCoupling( WheelPtr wheel );
        float computeWheelGearRPM( WheelPtr wheel );
        float computeEngineForceOnWheel( WheelPtr wheel, float deltaRPM, float coupling, float clampedThrottle );
        float computeEngineFriction( float deltaRPM, float clampedThrottle );
        float computeThrottleTransmission( float clampedThrottle );
        void updateEngineRPM( float deltaRPM, float throttleImpactOnRPM, float engineFriction );
        void updateWheel( WheelPtr wheel, float eForce, float eBreak );

        boost::recursive_mutex& mtx();
        void initPhysics();
        void updateWheelGeos();
        void updateEngine();
        void updateSpeedAndAcceleration();

        void addBTWheel(WheelPtr w);
        btRigidBody* createRigitBody(float mass, const btTransform& startTransform, btCollisionShape* shape);

        void updateChassis();

    public:
        VRCarDynamics(string name);
        ~VRCarDynamics();
        static VRCarDynamicsPtr create(string name);

        VRObjectPtr getRoot();
        VRTransformPtr getChassis();
        vector<VRTransformPtr> getWheels();

        float getClutch();
        float getThrottle();
        float getBreaking();
        float getSteering();
        int getGear();
        int getRPM();

        void addWheel(VRGeometryPtr geo, Vec3d p, float radius, float width, float maxSteering = 0, bool steered = false, bool driven = false);
        void setChassisGeo(VRTransformPtr geo, bool doPhys = 1);
        void setupSimpleWheels(VRTransformPtr geo, float xOffset, float frontZOffset, float rearZOffset, float height, float radius, float width, float maxSteering);
        void setType(TYPE type);
        void setParameter(float mass, float enginePower, float breakPower, Vec3d massOffset = Vec3d());

        void update(float throttle, float Break, float steering, float clutch = 0, int gear = 1);
        void updateWheel(int wheel, float throttle, float Break, float steering, float clutch = 0, int gear = 1);

        void reset(const Pose& p);
        float getSpeed();
        float getAcceleration();
        void setIgnition(bool b);
        bool isRunning();

        CarSoundPtr getCarSound();
};

OSG_END_NAMESPACE

#endif // CARDYNAMICS_H_INCLUDED
