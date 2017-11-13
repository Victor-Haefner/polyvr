#ifndef VRTRANSFORM_PHYSICS_EXT_H_INCLUDED
#define VRTRANSFORM_PHYSICS_EXT_H_INCLUDED

#include "core/utils/VRStorage.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"
#include <OpenSG/OSGMatrix.h>
#include <LinearMath/btVector3.h>
#include <LinearMath/btTransform.h>

class btRigidBody;
class btSoftBody;
class btSoftRigidDynamicsWorld;
class btPairCachingGhostObject;
class btCollisionShape;
class btDefaultMotionState;
class btCollisionObject;
class btManifoldPoint;
class btCollisionObjectWrapper;

using namespace std;

struct VRPhysicsJoint;

struct VRCollision {
    OSG::Vec3d pos1;
    OSG::Vec3d pos2;
    OSG::Vec3d norm;
    float distance;
    OSG::VRTransformWeakPtr obj1;
    OSG::VRTransformWeakPtr obj2;
};

class VRPhysics : public OSG::VRStorage {
    public:
        struct btCollision {
            btManifoldPoint* cp;
            btCollisionObjectWrapper* obj1;
            btCollisionObjectWrapper* obj2;
        };

        typedef VRFunction<btCollision> Callback;
        typedef std::shared_ptr<Callback> CallbackPtr;

    private:
        btRigidBody* body = 0;
        btPairCachingGhostObject* ghost_body = 0;
        /**soft body**/
        btSoftBody* soft_body = 0;

        btCollisionShape* shape = 0;
        btCollisionShape* customShape = 0;
        float shape_param = -1;
        btDefaultMotionState* motionState = 0;
        btSoftRigidDynamicsWorld* world = 0;
        int activation_mode = 0;
        int collisionGroup = 1;
        int collisionMask = 1;
        bool physicalized = false;
        bool dynamic = false;
        bool ghost = false;
        bool soft = false;
        bool physTree = true;
        bool useCallbacks = false;
        float mass = 1.0;
        float friction = 0.5;
        float collisionMargin = 0.04;
        float linDamping = 0;
        float angDamping = 0;
        btVector3 gravity = btVector3(0,-10,0);
        CallbackPtr callback;

        vector<OSG::Vec3d> torqueJob;
        vector<OSG::Vec3d> forceJob;
        vector<OSG::Vec3d> torqueJob2;
        vector<OSG::Vec3d> forceJob2;

        string comType = "geometric";
        OSG::Vec3d CoMOffset; // center of mass offset
        OSG::Vec3d CoMOffset_custom; // center of mass offset
        string physicsShape = "Convex";
        map<VRPhysics*, VRPhysicsJoint*> joints ;
        map<VRPhysics*, VRPhysicsJoint*> joints2;

        /** total force & torque added by addForce() or addTorque() in this frame **/
        btVector3 constantForce = btVector3(0,0,0);
        btVector3 constantTorque = btVector3(0,0,0);

        OSG::VRTransformWeakPtr vr_obj;
        OSG::VRGeometryPtr visShape;
        OSG::VRConstraintPtr constraint = 0;
        OSG::Vec3d scale = OSG::Vec3d(1,1,1);

        vector<OSG::VRGeometryPtr> getGeometries();

        btCollisionShape* getBoxShape();
        btCollisionShape* getSphereShape();
        btCollisionShape* getConvexShape(OSG::Vec3d& mc);
        btCollisionShape* getConcaveShape();
        btCollisionShape* getCompoundShape();
        btCollisionShape* getHACDShape();

        //btSoftBody*       createConvex();
        btSoftBody* createCloth();
        btSoftBody* createRope();
        void update();
        void clear();

        void updateVisualGeo();

    public:
        VRPhysics(OSG::VRTransformWeakPtr t);
        virtual ~VRPhysics();

        void prepareStep();

        btRigidBody* getRigidBody();
        btPairCachingGhostObject* getGhostBody();
        btCollisionObject* getCollisionObject();
        btCollisionShape* getCollisionShape();

        void setShape(string s, float param = -1);
        void setCustomShape(btCollisionShape* shape);
        string getShape();
        OSG::VRTransformPtr getVisualShape();

        void setPhysicalized(bool b);
        bool isPhysicalized();
        void physicalizeTree(bool b);
        void setCollisionCallback(CallbackPtr cb);

        void setDynamic(bool b);
        bool isDynamic();
        void setActivationMode(int m);
        int getActivationMode();

        void setGhost(bool b);
        bool isGhost();
        void setSoft(bool b);
        bool isSoft();

        void setMass(float m);
        float getMass();
        void setGravity(OSG::Vec3d v);

        void setFriction(float f);
        float getFriction();

        void setCollisionMargin(float m);
        void setCollisionGroup(int g);
        void setCollisionMask(int m);
        float getCollisionMargin();
        int getCollisionGroup();
        int getCollisionMask();

        vector<VRCollision> getCollisions();

        void updateTransformation(OSG::VRTransformPtr t);
        OSG::Matrix4d getTransformation();
        btTransform getTransform();
        void setTransformation(btTransform t);

        void pause(bool b = true);
        void resetForces();
        void applyImpulse(OSG::Vec3d i);
        void applyTorqueImpulse(OSG::Vec3d i);
        /** requests a force, which is handled in the physics thread later**/
        void addForce(OSG::Vec3d i);
        void addTorque(OSG::Vec3d i);

        void addConstantForce(OSG::Vec3d i);
        void addConstantTorque(OSG::Vec3d i);
        float getConstraintAngle(VRPhysics *to, int axis);
        void deleteConstraints(VRPhysics* with);
        /**get the requested total force in this frame **/
        OSG::Vec3d getForce();
        /** get requested total torque**/
        OSG::Vec3d getTorque();

        OSG::Vec3d getLinearVelocity();
        OSG::Vec3d getAngularVelocity();
        btMatrix3x3 getInertiaTensor();
        void setDamping(float lin,float ang);

        void setCenterOfMass(OSG::Vec3d com);

        static vector<string> getPhysicsShapes();
        static btTransform fromMatrix(OSG::Matrix4d m, OSG::Vec3d& scale, OSG::Vec3d mc);
        static btTransform fromMatrix(OSG::Matrix4d m, OSG::Vec3d mc);
        static btTransform fromVRTransform(OSG::VRTransformPtr t, OSG::Vec3d& scale, OSG::Vec3d mc);
        static OSG::Matrix4d fromBTTransform(const btTransform t);
        static OSG::Matrix4d fromBTTransform(const btTransform t, OSG::Vec3d& scale, OSG::Vec3d mc);

        static btVector3 toBtVector3(OSG::Vec3d);
        static OSG::Vec3d toVec3d(btVector3);


        void setConstraint(VRPhysics* p, OSG::VRConstraintPtr c, OSG::VRConstraintPtr cs); //for Rigid to Rigid
        void setConstraint(VRPhysics* p, int nodeIndex,OSG::Vec3d localPivot,bool ignoreCollision,float influence);//for Soft to Rigid

        void updateConstraint(VRPhysics* p);
        void updateConstraints();

        void triggerCallbacks(btManifoldPoint* cp, const btCollisionObjectWrapper* obj1, const btCollisionObjectWrapper* obj2 );
};

#endif // VRTRANSFORM_PHYSICS_EXT_H_INCLUDED
