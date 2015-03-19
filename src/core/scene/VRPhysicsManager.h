#ifndef VRPHYSICSMANAGER_H_INCLUDED
#define VRPHYSICSMANAGER_H_INCLUDED

#include <LinearMath/btAlignedObjectArray.h>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <map>
#include <vector>

template<class T> class VRFunction;

class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btRigidBody;
class btCollisionShape;
class btCollisionObject;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGeometry;
class VRTransform;
class VRVisualLayer;
class VRMaterial;
class VRThread;

class VRPhysicsManager {
     private:
        btBroadphaseInterface* broadphase;
        btDefaultCollisionConfiguration* collisionConfiguration;
        btCollisionDispatcher* dispatcher;
        btSequentialImpulseConstraintSolver* solver;
        btDiscreteDynamicsWorld* dynamicsWorld;

        btAlignedObjectArray<btCollisionShape*> collisionShapes;
        btRigidBody* body;

        map<btCollisionObject*, VRTransform*> OSGobjs;
        map<btCollisionObject*, VRGeometry*> physics_visuals;
        vector<btCollisionObject*> physics_visuals_to_update;
        VRVisualLayer* physics_visual_layer = 0;
        VRMaterial* phys_mat = 0;

        vector<Vec3f> collisionPoints;
        /** timestamp last frame**/
        int t_last;

    protected:
        VRFunction<VRThread*>* updatePhysicsFkt;
        VRFunction<int>* updatePhysObjectsFkt;
        VRPhysicsManager();
        ~VRPhysicsManager();

        void updatePhysics(VRThread* t);
        void updatePhysObjects();

    public:
        void physicalize(VRTransform* obj);
        void unphysicalize(VRTransform* obj);

        void setGravity(Vec3f g);

        btDiscreteDynamicsWorld* bltWorld();

        void collectCollisionPoints();
        vector<Vec3f>& getCollisionPoints();

        void setShowPhysics(bool b);
        bool getShowPhysics();
};

OSG_END_NAMESPACE;

#endif // VRPHYSICSMANAGER_H_INCLUDED
