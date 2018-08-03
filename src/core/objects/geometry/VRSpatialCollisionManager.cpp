#include "VRSpatialCollisionManager.h"
#include "core/math/boundingbox.h"
#include "core/math/Octree.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRPhysics.h"

#include <OpenSG/OSGTriangleIterator.h>
#include <btBulletDynamicsCommon.h>

using namespace OSG;

VRSpatialCollisionManager::VRSpatialCollisionManager(float resolution) : VRGeometry("spatialCollisionShape") {
    space = Octree::create(resolution);
}

VRSpatialCollisionManager::~VRSpatialCollisionManager() {}

VRSpatialCollisionManagerPtr VRSpatialCollisionManager::create(float resolution) { return VRSpatialCollisionManagerPtr( new VRSpatialCollisionManager(resolution) ); }

vector<VRGeometryPtr> tmpGeos;
void VRSpatialCollisionManager::add(VRObjectPtr o) {
    if (!o) return;
    cout << "VRSpatialCollisionManager::add " << o->getName() << endl;
    auto getCollisionShape = [&](Vec3d p) {
        //p = Vec3d(0,0,0);
        //cout << " getCollisionShape at: " << p << endl;
        auto node = space->get(p);
        if (node) {
            auto data = node->getData();
            if (data.size() > 0) return (btTriangleMesh*)data[0];
        }
        btTriangleMesh* tri_mesh = new btTriangleMesh();
        node = space->add(p, tri_mesh);
        //cout << " getCollisionShape, size: " << node->getSize() << " center: " << node->getCenter() << endl;
        return tri_mesh;
    };

    auto getTriPos = [](TriangleIterator& it, int i, Matrix4d& m) {
        Pnt3d p = Pnt3d( it.getPosition(i) );
        //m.mult(p,p);
        return p;
    };

    int N = 0;

    for (auto obj : o->getChildren(true, "Geometry", true)) {
        auto geo = dynamic_pointer_cast<VRGeometry>(obj);
        if (!geo) continue;
        if (geo->getMesh()->geo == 0) continue;
        //merge(geo);

        int N = 0;
        Matrix4d m;
        TriangleIterator ti(geo->getMesh()->geo);
        btVector3 vertexPos[3];
        while(!ti.isAtEnd()) {
            auto tri_mesh = getCollisionShape( Vec3d(getTriPos(ti, 0, m)) );

            for (int i=0;i<3;i++) {
                Pnt3d p = getTriPos(ti, i, m);
                for (int j=0;j<3;j++) vertexPos[i][j] = p[j];
            }

            tri_mesh->addTriangle(vertexPos[0], vertexPos[1], vertexPos[2]);
            ++ti;
            N++;
        }
    }
}

void VRSpatialCollisionManager::localize(Boundingbox box) {
    btCompoundShape* compound = new btCompoundShape();

    for (auto data : space->boxSearch(box)) {
        cout << "VRSpatialCollisionManager::localize " << data << endl;
        if (!data) continue;
        btTriangleMesh* tri_mesh = (btTriangleMesh*)data;
        btBvhTriangleMeshShape* shape = new btBvhTriangleMeshShape(tri_mesh, true);

        btTransform T;
        T.setIdentity();
        compound->addChildShape(T, shape);
    }

    getPhysics()->setDynamic(false);
    getPhysics()->setCustomShape(compound);
    getPhysics()->setPhysicalized(true);
    //setMeshVisibility(0);
}



