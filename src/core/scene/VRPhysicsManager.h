#ifndef VRPHYSICSMANAGER_H_INCLUDED
#define VRPHYSICSMANAGER_H_INCLUDED

#include <LinearMath/btAlignedObjectArray.h>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <map>
#include <vector>
#include <boost/thread/recursive_mutex.hpp>
#include "core/utils/VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"


template<class T> class VRFunction;

class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btSoftBodyRigidBodyCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;

class btDiscreteDynamicsWorld;

//TODO SOFT BODY
class btSoftRigidDynamicsWorld;
class btSoftBodyWorldInfo;

class btRigidBody;
class btCollisionShape;
class btCollisionObject;

class VRPhysics;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGeometry;
class VRTransform;
class VRVisualLayer;
class VRMaterial;
class VRThread;

class VRPhysicsManager {
     private:
        vector<VRFunction<int>* > updateFktsPre;
        vector<VRFunction<int>* > updateFktsPost;

        btBroadphaseInterface* broadphase;
        btSoftBodyRigidBodyCollisionConfiguration* collisionConfiguration;
        btCollisionDispatcher* dispatcher;
        btSequentialImpulseConstraintSolver* solver;
        //TODO SOFT BODY
        btSoftBodyWorldInfo* softBodyWorldInfo;
        btSoftRigidDynamicsWorld* dynamicsWorld;

        btAlignedObjectArray<btCollisionShape*> collisionShapes;
        btRigidBody* body;

        map<btCollisionObject*, VRTransformPtr> OSGobjs;
        map<btCollisionObject*, VRGeometryPtr> physics_visuals;
        vector<btCollisionObject*> physics_visuals_to_update;
        VRVisualLayer* physics_visual_layer = 0;
        VRMaterialPtr phys_mat = 0;

        boost::recursive_mutex mtx;
        boost::recursive_mutex lpmtx;
        boost::recursive_mutex namtx;

        long long getTime();
        int fps = 500;

    protected:
        VRFunction< std::weak_ptr<VRThread> >* updatePhysicsFkt;
        VRUpdatePtr updatePhysObjectsFkt;
        VRPhysicsManager();
        ~VRPhysicsManager();

        void prepareObjects();
        void updatePhysics( std::weak_ptr<VRThread>  t);
        void updatePhysObjects();

    public:
        void physicalize(VRTransformPtr obj);
        void unphysicalize(VRTransformPtr obj);

        void addPhysicsUpdateFunction(VRFunction<int>* fkt, bool after);
        void dropPhysicsUpdateFunction(VRFunction<int>* fkt, bool after);

        void setGravity(Vec3f g);

        btSoftRigidDynamicsWorld* bltWorld();
        btSoftBodyWorldInfo* getSoftBodyWorldInfo();

        VRVisualLayer* getVisualLayer();

        boost::recursive_mutex& physicsMutex();
        boost::recursive_mutex& lowPriorityMutex();
        boost::recursive_mutex& nextAccessMutex();
};

OSG_END_NAMESPACE;

#endif // VRPHYSICSMANAGER_H_INCLUDED
