#if !defined(VRPHYSICSMANAGER_H_INCLUDED) && !defined(WITHOUT_BULLET)
#define VRPHYSICSMANAGER_H_INCLUDED

#include <LinearMath/btAlignedObjectArray.h>
#include <OpenSG/OSGConfig.h>
#include <map>
#include <vector>
#include "core/math/OSGMathFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRUtilsFwd.h"
#include "core/objects/VRObjectFwd.h"

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

namespace boost { class recursive_mutex; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRPhysics;
class VRGeometry;
class VRTransform;
class VRVisualLayer;
class VRMaterial;
class VRThread;

class VRPhysicsManager {
     private:
        vector<VRUpdateCbWeakPtr > updateFktsPre;
        vector<VRUpdateCbWeakPtr > updateFktsPost;
        bool active = 1;
        bool skip = 1;

        btBroadphaseInterface* broadphase;
        btSoftBodyRigidBodyCollisionConfiguration* collisionConfiguration;
        btCollisionDispatcher* dispatcher;
        btSequentialImpulseConstraintSolver* solver;
        //TODO SOFT BODY
        btSoftBodyWorldInfo* softBodyWorldInfo;
        btSoftRigidDynamicsWorld* dynamicsWorld;

        btAlignedObjectArray<btCollisionShape*> collisionShapes;
        btRigidBody* body;

        map<btCollisionObject*, VRTransformWeakPtr> OSGobjs;
        map<btCollisionObject*, VRGeometryPtr> physics_visuals;
        vector<btCollisionObject*> physics_visuals_to_update;
        VRVisualLayerPtr physics_visual_layer;
        VRMaterialPtr phys_mat = 0;

        boost::recursive_mutex* mtx = 0;
        int fps = 500;

    protected:
        VRThreadCbPtr updatePhysicsFkt;
        VRUpdateCbPtr updatePhysObjectsFkt;
        VRPhysicsManager();
        ~VRPhysicsManager();

        void prepareObjects();
        void updatePhysics( VRThreadWeakPtr  t);
        void updatePhysObjects();

    public:
        void physicalize(VRTransformPtr obj);
        void unphysicalize(VRTransformPtr obj);

        void addPhysicsUpdateFunction(VRUpdateCbPtr fkt, bool after);
        void dropPhysicsUpdateFunction(VRUpdateCbPtr fkt, bool after);

        void setPhysicsActive(bool a);
        void setGravity(Vec3d g);

        btSoftRigidDynamicsWorld* bltWorld();
        btSoftBodyWorldInfo* getSoftBodyWorldInfo();

        VRVisualLayerPtr getVisualLayer();

        boost::recursive_mutex& physicsMutex();
};

OSG_END_NAMESPACE;

#endif // VRPHYSICSMANAGER_H_INCLUDED
