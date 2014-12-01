#ifndef VRTRANSFORM_PHYSICS_EXT_H_INCLUDED
#define VRTRANSFORM_PHYSICS_EXT_H_INCLUDED

#include "core/utils/VRStorage.h"
#include <btBulletDynamicsCommon.h>
#include <OpenSG/OSGMatrix.h>

using namespace std;

namespace OSG{ class VRTransform; }
namespace OSG{ class VRConstraint; }
class VRPhysicsJoint;

class VRPhysics : public OSG::VRStorage {
    private:
        btRigidBody* body;
        btCollisionShape* shape;
        btDefaultMotionState* motionState;
        btDiscreteDynamicsWorld* world;
        int activation_mode;
        int collisionGroup;
        int collisionMask;
        bool physicalized;
        bool dynamic;
        float mass;
        float collisionMargin;
        string physicsShape;
        map<VRPhysics*, VRPhysicsJoint*> joints ;
        map<VRPhysics*, VRPhysicsJoint*> joints2;
        map<VRPhysics*, VRPhysicsJoint*>::iterator jointItr;

        OSG::VRTransform* vr_obj;
        OSG::VRConstraint* constraint;

        btCollisionShape* getBoxShape();
        btCollisionShape* getSphereShape();
        btCollisionShape* getConvexShape();
        btCollisionShape* getConcaveShape();

        void update();

    public:
        VRPhysics(OSG::VRTransform* t);
        ~VRPhysics();

        btRigidBody* obj();

        void setShape(string s);
        string getShape();

        void setPhysicalized(bool b);
        bool isPhysicalized();

        void setDynamic(bool b);
        bool isDynamic();
        void setActivationMode(int m);
        int getActivationMode();

        void setMass(float m);
        float getMass();

        void setCollisionMargin(float m);
        void setCollisionGroup(int g);
        void setCollisionMask(int m);
        float getCollisionMargin();
        int getCollisionGroup();
        int getCollisionMask();

        void updateTransformation(const OSG::Matrix& m);
        OSG::Matrix getTransformation();
        void setTransformation(btTransform t);

        void pause(bool b = true);
        void resetForces();
        void applyImpulse(OSG::Vec3f i);
        void applyForce(OSG::Vec3f i);

        /**get the normalized resulting force (out of a collision, without the gravity) of this object with the constraints considered**/
        btVector3 getNormForceWithConstrained();
        /**get the resulting force (out of a collision, without the gravity) of the given object**/
        btVector3 getForce();


        static vector<string> getPhysicsShapes();
        static btTransform fromMatrix(const OSG::Matrix& m);
        static OSG::Matrix fromTransform(const btTransform t);

        void setConstraint(VRPhysics* p, OSG::VRConstraint* c, OSG::VRConstraint* cs);
        void updateConstraint(VRPhysics* p);
        void updateConstraints();
};

#endif // VRTRANSFORM_PHYSICS_EXT_H_INCLUDED
