#include "VRSpatialCollisionManager.h"
#include "core/math/boundingbox.h"
#include "core/math/Octree.h"
#include "core/math/pose.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/scene/VRScene.h"

#include <OpenSG/OSGTriangleIterator.h>
#include <btBulletDynamicsCommon.h>

using namespace OSG;


VRSpatialCollisionManager::VRSpatialCollisionManager(float resolution) : VRGeometry("spatialCollisionShape") {
    space = Octree::create(resolution,10,"spatialCollisionShape");
    hide("SHADOW");
    updateCollisionCb = VRUpdateCb::create( "collision check", bind( &VRSpatialCollisionManager::checkCollisions, this ) );
    VRScene::getCurrent()->addUpdateFkt(updateCollisionCb);
}

VRSpatialCollisionManager::~VRSpatialCollisionManager() {}
VRSpatialCollisionManagerPtr VRSpatialCollisionManager::create(float resolution) { return VRSpatialCollisionManagerPtr( new VRSpatialCollisionManager(resolution) ); }

void VRSpatialCollisionManager::checkCollisions() {
    if (!collisionCb) return;

    /*auto getCollisionTriangle = [&](Vec3d pos, vector<Vec4d>& triangle, int triangleID) {
        btTriangleMesh* t = getCollisionShape(pos, false);
        if (t) {
            IndexedMeshArray& mesh = t->getIndexedMeshArray();
            if (mesh.size() > 1) cout << "VRSpatialCollisionManager::checkCollisions, WARNING: more than one mesh! " << mesh.size() << endl;
            if (mesh.size() > 0) {
                int Ni = mesh[0].m_numTriangles;
                int Nv = mesh[0].m_numVertices;
                if (triangleID >= Ni) cout << "VRSpatialCollisionManager::checkCollisions, WARNING: triangleID " << triangleID << " to big! (" << Ni << ") N mesh: " << mesh.size() << endl;
                else {
                    unsigned int* bt_inds = (unsigned int*)mesh[0].m_triangleIndexBase;
                    btVector3* verts = (btVector3*)mesh[0].m_vertexBase;
                    btVector3 vert1 = verts[bt_inds[triangleID*3+0]]; // first trianlge vertex
                    btVector3 vert2 = verts[bt_inds[triangleID*3+1]]; // secon trianlge vertex
                    btVector3 vert3 = verts[bt_inds[triangleID*3+2]]; // third trianlge vertex
                    triangle = vector<Vec4d>( { VRPhysics::toVec4d(vert1), VRPhysics::toVec4d(vert2), VRPhysics::toVec4d(vert3) } );
                }
            }
        }
    };*/

    auto collisions = getCollisions();
    Vec3d wp = getWorldPosition();
    for (auto& c : collisions) {
        c.pos1 -= wp;
        c.pos2 -= wp;
        //getCollisionTriangle(c.pos1, c.triangle1, c.triangleID1);
        //getCollisionTriangle(c.pos2, c.triangle2, c.triangleID2);
    }
    if (collisions.size()) (*collisionCb)(collisions);
    //int objID = co;
    //cout << " VRSpatialCollisionManager::checkCollisions: " << toVec3d(v) << endl;
}

void VRSpatialCollisionManager::setCollisionCallback(VRCollisionCbPtr cb) { collisionCb = cb; }

btTriangleMesh* VRSpatialCollisionManager::getCollisionShape(Vec3d p, bool create) {
    //p = Vec3d(0,0,0);
    //cout << " getCollisionShape at: " << p << endl;
    auto node = space->get(p);
    if (node) {
        auto data = node->getData();
        if (data.size() > 0) return (btTriangleMesh*)data[0];
    }
    if (!create) return 0;
    btTriangleMesh* tri_mesh = new btTriangleMesh();
    node = space->add(p, tri_mesh);
    //cout << " getCollisionShape, size: " << node->getSize() << " center: " << node->getCenter() << endl;
    return tri_mesh;
};

vector<VRGeometryPtr> tmpGeos;
void VRSpatialCollisionManager::add(VRObjectPtr o, int objID) {
    if (!o) return;
    //objID = VRScene::getCurrent()->getRoot()->getID();
    //cout << "VRSpatialCollisionManager::add " << o->getName() << endl;

    auto getTriPos = [](TriangleIterator& it, int i, Matrix4d& m) {
        Pnt3d p = Pnt3d( it.getPosition(i) );
        //m.mult(p,p);
        return p;
    };

    for (auto obj : o->getChildrenWithTag("geometry", true, true)) {
        auto geo = dynamic_pointer_cast<VRGeometry>(obj);
        if (!geo) continue;
        if (geo->getMesh() == 0) continue;
        if (geo->getMesh()->geo == 0) continue;
        //merge(geo); // enable for debugging purposes

        int N = 0;
        Matrix4d m;
        TriangleIterator ti(geo->getMesh()->geo);
        btVector3 vertexPos[3];
        while(!ti.isAtEnd()) {
            auto tri_mesh = getCollisionShape( Vec3d(getTriPos(ti, 0, m)) );

            for (int i=0;i<3;i++) {
                Pnt3d p = getTriPos(ti, i, m);
                for (int j=0;j<3;j++) vertexPos[i][j] = p[j];
                vertexPos[i].setW(objID);
            }

            tri_mesh->addTriangle(vertexPos[0], vertexPos[1], vertexPos[2]);
            ++ti;
            N++;
        }
    }
}

void VRSpatialCollisionManager::addQuad(float width, float height, Pose& p, int objID) {
    Vec3d pos = p.pos() + p.up()*height*0.5;
    VRGeoData data; data.pushQuad();
    data.pushQuad(pos, p.dir(), p.up(), Vec2d(width, height), true);
    add(data.asGeometry("tmp"), objID);
}

void VRSpatialCollisionManager::localize(Boundingbox box) {
    /*auto old_shape = getPhysics()->getCollisionShape();
    if (old_shape) {
        btBvhTriangleMeshShape* c = (btBvhTriangleMeshShape*)old_shape;
        auto tmsh = (btTriangleMesh*)c->getMeshInterface();
        tmsh->getIndexedMeshArray().clear();
        //for (int i=0; i<tmsh->getNumParts(); i++) tmsh->removePart(i);
    }*/

    btTriangleMesh* mesh = new btTriangleMesh();
    bool valid = false;

    for (auto data : space->boxSearch(box)) {
        if (!data) continue;
        btTriangleMesh* tri_mesh = (btTriangleMesh*)data;
        auto& meshes = tri_mesh->getIndexedMeshArray();
        for (int i=0; i<meshes.size(); i++ ) {
            if (meshes[i].m_numTriangles > 0) {
                mesh->addIndexedMesh( meshes[i] );
                valid = true;
            }
        }
    }

    if (!valid) return;

    btBvhTriangleMeshShape* shape = new btBvhTriangleMeshShape(mesh, true);
    getPhysics()->setPhysicalized(false);
    getPhysics()->setDynamic(false);
    getPhysics()->setCustomShape(shape);
    getPhysics()->setPhysicalized(true);
}







