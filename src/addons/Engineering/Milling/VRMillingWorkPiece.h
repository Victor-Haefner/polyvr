#ifndef VRMILLINGWORKPIECE_H_INCLUDED
#define VRMILLINGWORKPIECE_H_INCLUDED

#include <string>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include "core/objects/geometry/VRGeometry.h"
#include "core/math/Octree.h"

OSG_BEGIN_NAMESPACE;

class VRWorkpieceElement {
private:
    VRGeometryPtr anchor;
    VRWorkpieceElement* children[2];
    VRWorkpieceElement* parent;

    Vec3i blocks;
    Vec3f size;
    Vec3f offset;
    Vec3f position;
    bool deleted;

    const float blockSize;
    const int geometryCreateLevel;
    const int maxTreeLevel;
    const int level;
    bool updateIssued;

    VRGeometryPtr geometry;
    GeoPnt3fPropertyRecPtr positions;
    GeoVec3fPropertyRecPtr normals;
    GeoUInt32PropertyRecPtr indices;

    static Vec3f mulVec3f(Vec3f lhs, Vec3f rhs);

public:
    VRWorkpieceElement(VRGeometryPtr anchor, VRWorkpieceElement* parent,
                       Vec3i blocks, Vec3f size,
                       Vec3f offset, Vec3f position,
                       const float blockSize, const int geometryCreateLevel, const int maxTreeLevel,
                       const int level);

    // 6 sides with 4 vertices each
    static const int verticesPerElement = 6 * 4;
    static const Vec3f planeOffsetMasks[2][3];
    static const Vec3f vertexOffsetMasks[2][3][4];
    static const float sign[2];

    /*
     * deletes all children with their geometries
     */
    ~VRWorkpieceElement();
    void deleteGeometry();
    void issueGeometryUpdate(int level);
    void split();
    bool collides(Vec3f position);
    bool collide(Vec3f position);
    void deleteElement();
    bool isDeleted() const;
    void build();
    void build(GeoPnt3fPropertyRecPtr positions,
               GeoVec3fPropertyRecPtr normals,
               GeoUInt32PropertyRecPtr indices,
               uint32_t& index);
    void buildGeometry(GeoPnt3fPropertyRecPtr positions,
                       GeoVec3fPropertyRecPtr normals,
                       GeoUInt32PropertyRecPtr indices,
                       uint32_t& index);

};

class VRMillingWorkPiece : public VRGeometry {
    private:
        Vec3i gridSize;
        float blockSize = 0.01;
        int lastToolChange = 0;
        int updateCount = 0;
        pose toolPose;
        VRTransformWeakPtr tool;
        VRUpdatePtr uFkt;

        int levelsPerGeometry = 12; // can be overridden
        int geometryUpdateWait = 1;

        void update();

        VRWorkpieceElement* rootElement;

    public:
        VRMillingWorkPiece(string name);
        VRMillingWorkPiecePtr ptr();
        static VRMillingWorkPiecePtr create(string name);
        void init(Vec3i gSize, float bSize = 0.01);
        void reset();
        void setCuttingTool(VRTransformPtr geo);
        void updateGeometry();
        void setLevelsPerGeometry(int levels);  // this will take effect after the next reset
        void setRefreshWait(int updatesToWait); // this will take effect immediately
};

OSG_END_NAMESPACE;

#endif // VRMILLINGWORKPIECE_H_INCLUDED
