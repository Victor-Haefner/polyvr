#ifndef VRTRANSFORM_PHYSICS_EXT_H_INCLUDED
#define VRTRANSFORM_PHYSICS_EXT_H_INCLUDED

#include "addons/Bullet/VRPhysicsFwd.h"
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

struct VRPhysicsJoint;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRCollision {
    public:
        Vec3d pos1;
        Vec3d pos2;
        Vec3d norm;
        float distance;
        VRTransformWeakPtr obj1;
        VRTransformWeakPtr obj2;
        int triangleID1;
        int triangleID2;
        vector<Vec4d> triangle1;
        vector<Vec4d> triangle2;

    public:
        VRCollision();
        ~VRCollision();

        Vec3d getPos1();
        Vec3d getPos2();
        Vec3d getNorm();
        float getDistance();
        VRTransformPtr getObj1();
        VRTransformPtr getObj2();
        vector<Vec4d> getTriangle1();
        vector<Vec4d> getTriangle2();
};

class VRPhysics : public VRStorage {
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
        bool paused = false;
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

        // convex decomposition parameters
        float compacityWeight = 0.1;
        float volumeWeight = 0.0;
        float NClusters = 2;
        float NVerticesPerCH = 100;
        float concavity = 100;
        bool addExtraDistPoints = false;
        bool addNeighboursDistPoints = false;
        bool addFacesPoints = false;

        vector<Vec3d> torqueJob;
        vector<Vec3d> forceJob;
        vector<Vec3d> torqueJob2;
        vector<Vec3d> forceJob2;

        string comType = "geometric";
        Vec3d CoMOffset; // center of mass offset
        Vec3d CoMOffset_custom; // center of mass offset
        string physicsShape = "Convex";
        map<VRPhysics*, VRPhysicsJoint*> joints ;
        map<VRPhysics*, VRPhysicsJoint*> joints2;

        /** total force & torque added by addForce() or addTorque() in this frame **/
        btVector3 constantForce = btVector3(0,0,0);
        btVector3 constantTorque = btVector3(0,0,0);

        VRTransformWeakPtr vr_obj;
        VRGeometryPtr visShape;
        VRConstraintPtr constraint = 0;
        Vec3d scale = Vec3d(1,1,1);
        btVector3 inertia = btVector3(0,0,0);

        vector<VRGeometryPtr> getGeometries();

        btCollisionShape* getBoxShape();
        btCollisionShape* getSphereShape();
        btCollisionShape* getConvexShape(Vec3d& mc);
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
        VRPhysics(VRTransformWeakPtr t);
        virtual ~VRPhysics();

        void prepareStep();

        btRigidBody* getRigidBody();
        btPairCachingGhostObject* getGhostBody();
        btCollisionObject* getCollisionObject();
        btCollisionShape* getCollisionShape();
        Vec3d getCenterOfMass();

        void setShape(string s, float param = -1);
        void setCustomShape(btCollisionShape* shape);
        string getShape();
        VRTransformPtr getVisualShape();
        void setConvexDecompositionParameters(float cw, float vw, float nc, float nv, float c, bool aedp, bool andp, bool afp);

        void setPhysicalized(bool b);
        bool isPhysicalized();
        void physicalizeTree(bool b);
        void setCollisionCallback(CallbackPtr cb);

        void setDynamic(bool b, bool fast = false);
        bool isDynamic();
        void setActivationMode(int m);
        int getActivationMode();

        void setGhost(bool b);
        bool isGhost();
        void setSoft(bool b);
        bool isSoft();

        void setMass(float m);
        float getMass();
        void setGravity(Vec3d v);

        void setFriction(float f);
        float getFriction();

        void setCollisionMargin(float m);
        void setCollisionGroup(int g);
        void setCollisionMask(int m);
        float getCollisionMargin();
        int getCollisionGroup();
        int getCollisionMask();

        vector<VRCollision> getCollisions();

        void updateTransformation(VRTransformPtr t);
        Matrix4d getTransformation();
        btTransform getTransform();
        void setTransformation(btTransform t);

        void pause(bool b = true);
        void resetForces();
        void applyImpulse(Vec3d i);
        void applyTorqueImpulse(Vec3d i);
        /** requests a force, which is handled in the physics thread later**/
        void addForce(Vec3d i);
        void addTorque(Vec3d i);

        void addConstantForce(Vec3d i);
        void addConstantTorque(Vec3d i);
        float getConstraintAngle(VRPhysics *to, int axis);
        void deleteConstraints(VRPhysics* with);
        /**get the requested total force in this frame **/
        Vec3d getForce();
        /** get requested total torque**/
        Vec3d getTorque();

        Vec3d getLinearVelocity();
        Vec3d getAngularVelocity();
        btMatrix3x3 getInertiaTensor();
        void setDamping(float lin,float ang);

        void setCenterOfMass(Vec3d com);

        static vector<string> getPhysicsShapes();
        static btTransform fromMatrix(Matrix4d m, Vec3d& scale, Vec3d mc);
        static btTransform fromMatrix(Matrix4d m, Vec3d mc);
        static btTransform fromVRTransform(VRTransformPtr t, Vec3d& scale, Vec3d mc);
        static Matrix4d fromBTTransform(const btTransform t);
        static Matrix4d fromBTTransform(const btTransform t, Vec3d& scale, Vec3d mc);

        static btVector3 toBtVector3(Vec3d);
        static Vec3d toVec3d(btVector3);
        static btVector3 toBtVector3(Vec4d);
        static Vec4d toVec4d(btVector3);

        void setConstraint(VRPhysics* p, VRConstraintPtr c, VRConstraintPtr cs = 0); //for Rigid to Rigid
        void setConstraint(VRPhysics* p, int nodeIndex,Vec3d localPivot,bool ignoreCollision,float influence);//for Soft to Rigid
        void setSpringParameters(VRPhysics* p, int dof, float stiffnes, float damping);

        void updateConstraint(VRPhysics* p);
        void updateConstraints();

        void triggerCallbacks(btManifoldPoint* cp, const btCollisionObjectWrapper* obj1, const btCollisionObjectWrapper* obj2 );
};

OSG_END_NAMESPACE;

#endif // VRTRANSFORM_PHYSICS_EXT_H_INCLUDED
