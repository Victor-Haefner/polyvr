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
    OSG::Vec3f pos1;
    OSG::Vec3f pos2;
    OSG::Vec3f norm;
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
        float shape_param = -1;
        btDefaultMotionState* motionState = 0;
        btSoftRigidDynamicsWorld* world = 0;
        int activation_mode = 0;
        int collisionGroup = 0;
        int collisionMask = 0;
        bool physicalized = false;
        bool dynamic = false;
        bool ghost = false;
        bool soft = false;
        bool physTree = true;
        bool useCallbacks = false;
        float mass = 1.0;
        float collisionMargin = 0.3;
        float linDamping = 0;
        float angDamping = 0;
        btVector3 gravity;
        CallbackPtr callback;

        vector<OSG::Vec3f> torqueJob;
        vector<OSG::Vec3f> forceJob;
        vector<OSG::Vec3f> torqueJob2;
        vector<OSG::Vec3f> forceJob2;

        string comType = "geometric";
        OSG::Vec3f CoMOffset; // center of mass offset
        OSG::Vec3f CoMOffset_custom; // center of mass offset
        string physicsShape;
        map<VRPhysics*, VRPhysicsJoint*> joints ;
        map<VRPhysics*, VRPhysicsJoint*> joints2;

        /** total force & torque added by addForce() or addTorque() in this frame **/
        btVector3 constantForce;
        btVector3 constantTorque;

        OSG::VRTransformWeakPtr vr_obj;
        OSG::VRGeometryPtr visShape;
        OSG::VRConstraintPtr constraint = 0;
        OSG::Vec3f scale;

        vector<OSG::VRGeometryPtr> getGeometries();

        btCollisionShape* getBoxShape();
        btCollisionShape* getSphereShape();
        btCollisionShape* getConvexShape(OSG::Vec3f& mc);
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
        void setGravity(OSG::Vec3f v);

        void setCollisionMargin(float m);
        void setCollisionGroup(int g);
        void setCollisionMask(int m);
        float getCollisionMargin();
        int getCollisionGroup();
        int getCollisionMask();

        vector<VRCollision> getCollisions();

        void updateTransformation(OSG::VRTransformWeakPtr t);
        OSG::Matrix getTransformation();
        btTransform getTransform();
        void setTransformation(btTransform t);

        void pause(bool b = true);
        void resetForces();
        void applyImpulse(OSG::Vec3f i);
        void applyTorqueImpulse(OSG::Vec3f i);
        /** requests a force, which is handled in the physics thread later**/
        void addForce(OSG::Vec3f i);
        void addTorque(OSG::Vec3f i);

        void addConstantForce(OSG::Vec3f i);
        void addConstantTorque(OSG::Vec3f i);
        float getConstraintAngle(VRPhysics *to, int axis);
        void deleteConstraints(VRPhysics* with);
        /**get the requested total force in this frame **/
        OSG::Vec3f getForce();
        /** get requested total torque**/
        OSG::Vec3f getTorque();

        OSG::Vec3f getLinearVelocity();
        OSG::Vec3f getAngularVelocity();
        btMatrix3x3 getInertiaTensor();
        void setDamping(float lin,float ang);

        void setCenterOfMass(OSG::Vec3f com);

        static vector<string> getPhysicsShapes();
        static btTransform fromMatrix(OSG::Matrix m, OSG::Vec3f& scale, OSG::Vec3f mc);
        static btTransform fromMatrix(OSG::Matrix m, OSG::Vec3f mc);
        static btTransform fromVRTransform(OSG::VRTransformWeakPtr t, OSG::Vec3f& scale, OSG::Vec3f mc);
        static OSG::Matrix fromBTTransform(const btTransform t);
        static OSG::Matrix fromBTTransform(const btTransform t, OSG::Vec3f& scale, OSG::Vec3f mc);

        static btVector3 toBtVector3(OSG::Vec3f);
        static OSG::Vec3f toVec3f(btVector3);


        void setConstraint(VRPhysics* p, OSG::VRConstraintPtr c, OSG::VRConstraintPtr cs); //for Rigid to Rigid
        void setConstraint(VRPhysics* p, int nodeIndex,OSG::Vec3f localPivot,bool ignoreCollision,float influence);//for Soft to Rigid

        void updateConstraint(VRPhysics* p);
        void updateConstraints();

        void triggerCallbacks(btManifoldPoint* cp, const btCollisionObjectWrapper* obj1, const btCollisionObjectWrapper* obj2 );
};

#endif // VRTRANSFORM_PHYSICS_EXT_H_INCLUDED
