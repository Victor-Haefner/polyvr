#ifndef VRTRANSFORM_PHYSICS_EXT_H_INCLUDED
#define VRTRANSFORM_PHYSICS_EXT_H_INCLUDED

#include "core/utils/VRStorage.h"
#include <btBulletDynamicsCommon.h>
#include <OpenSG/OSGMatrix.h>

using namespace std;

namespace OSG{ class VRTransform; }
namespace OSG{ class VRConstraint; }
class VRPhysicsJoint;
class btPairCachingGhostObject;

struct VRCollision {
    OSG::Vec3f pos1;
    OSG::Vec3f pos2;
    OSG::Vec3f norm;
    float distance;
    OSG::VRTransform* obj1 = 0;
    OSG::VRTransform* obj2 = 0;
};

class VRPhysics : public OSG::VRStorage {
    private:
        btRigidBody* body = 0;
        btPairCachingGhostObject* ghost_body = 0;
        btCollisionShape* shape = 0;
        float shape_param = -1;
        btDefaultMotionState* motionState = 0;
        btDiscreteDynamicsWorld* world = 0;
        int activation_mode = 0;
        int collisionGroup = 0;
        int collisionMask = 0;
        bool physicalized = false;
        bool dynamic = false;
        bool ghost = false;
        float mass = 1.0;
        float collisionMargin = 0.3;
        string physicsShape;
        map<VRPhysics*, VRPhysicsJoint*> joints ;
        map<VRPhysics*, VRPhysicsJoint*> joints2;
        map<VRPhysics*, VRPhysicsJoint*>::iterator jointItr;

        OSG::VRTransform* vr_obj = 0;
        OSG::VRConstraint* constraint = 0;

        btCollisionShape* getBoxShape();
        btCollisionShape* getSphereShape();
        btCollisionShape* getConvexShape();
        btCollisionShape* getConcaveShape();

        OSG::Vec3f toVec3f(btVector3);

        void update();

    public:
        VRPhysics(OSG::VRTransform* t);
        ~VRPhysics();

        btRigidBody* obj();

        void setShape(string s, float param = -1);
        string getShape();

        void setPhysicalized(bool b);
        bool isPhysicalized();

        void setDynamic(bool b);
        bool isDynamic();
        void setActivationMode(int m);
        int getActivationMode();

        void setGhost(bool b);
        bool isGhost();

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

        void updateTransformation(const OSG::Matrix& m);
        OSG::Matrix getTransformation();
        btTransform getTransform();
        void setTransformation(btTransform t);

        void pause(bool b = true);
        void resetForces();
        void applyImpulse(OSG::Vec3f i);
        void addForce(OSG::Vec3f i);
        void addTorque(OSG::Vec3f i);

        /**get the normalized resulting force (out of a collision, without the gravity) of this object with the constraints considered**/
      //  btVector3 getNormForceWithConstrained();
        /**get the resulting force (out of a collision, without the gravity) of the given object**/
        OSG::Vec3f getForce();

        OSG::Vec3f getLinearVelocity();
        OSG::Vec3f getAngularVelocity();
        btMatrix3x3 getInertiaTensor();
        void setDamping(float lin,float ang);






        static vector<string> getPhysicsShapes();
        static btTransform fromMatrix(const OSG::Matrix& m);
        static OSG::Matrix fromTransform(const btTransform t);


        void setConstraint(VRPhysics* p, OSG::VRConstraint* c, OSG::VRConstraint* cs);
        void updateConstraint(VRPhysics* p);
        void updateConstraints();
};

#endif // VRTRANSFORM_PHYSICS_EXT_H_INCLUDED
