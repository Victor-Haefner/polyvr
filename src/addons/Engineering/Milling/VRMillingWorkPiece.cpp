#include "VRMillingWorkPiece.h"
#include "core/scene/VRScene.h"
#include "core/math/pose.h"

using namespace OSG;

VRMillingWorkPiece::VRMillingWorkPiece(string name) : VRGeometry(name), rootElement(nullptr) {
	type = "MillingWorkPiece";
	uFkt = VRUpdateCb::create("MillingWorkPiece-update", bind(&VRMillingWorkPiece::update, this));
	VRScene::getCurrent()->addUpdateFkt(uFkt);
}

VRMillingWorkPiecePtr VRMillingWorkPiece::ptr() { return static_pointer_cast<VRMillingWorkPiece>( shared_from_this() ); }
VRMillingWorkPiecePtr VRMillingWorkPiece::create(string name) { return shared_ptr<VRMillingWorkPiece>(new VRMillingWorkPiece(name) ); }

void VRMillingWorkPiece::setCuttingTool(VRTransformPtr geo) {
    tool = geo;
    toolPose = geo->getWorldPose();
}

void VRMillingWorkPiece::setCuttingProfile(shared_ptr<VRMillingCuttingToolProfile> profile) {
    cuttingProfile = profile;
}

void VRMillingWorkPiece::init(Vec3i gSize, float bSize) {
    gridSize = gSize;
    blockSize = bSize;

    // update the position of the workpiece
    this->getWorldPosition();

    // idea: do not delete the workpiece just
    // set the deleted flags of the elements to false.
    if (rootElement != nullptr) {
        delete rootElement;
        rootElement = nullptr;
    }

    int maxElements = gSize[0] * gSize[1] * gSize[2];
    maxTreeLevel = (int) std::log2(maxElements);
    geometryCreateLevel = 0;

    if (levelsPerGeometry < maxTreeLevel) {
        geometryCreateLevel = maxTreeLevel - levelsPerGeometry;
    }

    rootElement = new VRWorkpieceElement(*this, (VRWorkpieceElement*) nullptr,
                                  Vec3i(gridSize), Vec3d(gridSize) * blockSize,
                                  Vec3d(0, 0, 0), getWorldPosition(), 0);

    rootElement->build();
}

void VRMillingWorkPiece::reset() {
    init(gridSize, blockSize);
}

void VRMillingWorkPiece::update() {
    Vec3d toolPosition;

    if (cuttingProfile == nullptr) {
        return;
    }

    { // locking scope
        auto geo = tool.lock();
        if (!geo) return;
        if (!geo->changedNow()) return; // keine bewegung

        toolPosition = geo->getWorldPosition();
    }

    if (!rootElement->collide(toolPosition)) {
        return;
    }

    if (updateCount++ % geometryUpdateWait == 0) {
        rootElement->build();
    }
}

void VRMillingWorkPiece::updateGeometry() {
    rootElement->build();
}

void VRMillingWorkPiece::setLevelsPerGeometry(int levels) {
    if (levels > 0) {
        this->levelsPerGeometry = levels;
    }
}

void VRMillingWorkPiece::setRefreshWait(int updatesToWait) {
    if (updatesToWait > 0) {
        this->geometryUpdateWait = updatesToWait;
    }
}

/*
* workpiece element defitions
*/

VRWorkpieceElement::VRWorkpieceElement(VRMillingWorkPiece& workpiece, VRWorkpieceElement* parent,
                                       Vec3i blocks, Vec3d size, Vec3d offset, Vec3d position, const int level)
    : workpiece(workpiece), children(), parent(parent), blocks(blocks), size(size), offset(offset),
      position(position), level(level), deleted(false), updateIssued(true)
{}

VRWorkpieceElement::~VRWorkpieceElement() {
    deleteGeometry();
    deleteChildren();
}

void VRWorkpieceElement::deleteGeometry() {
    if (geometry != nullptr) {
        geometry->destroy();
        geometry.reset();
        geometry = nullptr;
    }
}

void VRWorkpieceElement::deleteChildren() {
    if (children[0] != nullptr) {
        delete children[0];
        delete children[1];
        // delete does not reset the pointer
        children[0] = nullptr;
        children[1] = nullptr;
    }
}

void VRWorkpieceElement::issueGeometryUpdate() {
    if (parent != nullptr) {
        if (level > workpiece.geometryCreateLevel) {
            parent->issueGeometryUpdate();
        }
        else {
            updateIssued = true;
        }
    }
}

void VRWorkpieceElement::split() {
    if (children[0] != nullptr) {
        return;
    }

    // find greatest dimension
    int divDim = 0;
    for (int i = 1; i < 3; i++) {
        if (blocks[i] > blocks[divDim]) {
            divDim = i;
        }
    }

    int blocksDim = blocks[divDim];

    // abort, when no further split is possible
    if (blocksDim == 1) return;

    int blocksLeft = blocksDim / 2; // integer division
    int blocksRight = blocksDim - blocksLeft;
    float lengthLeft = blocksLeft * workpiece.blockSize;
    float lengthRight = blocksRight * workpiece.blockSize;
    float centerOld = blocksDim * workpiece.blockSize / 2.0f;
    float centerLeftOffset = (lengthLeft / 2.0f) - centerOld;
    float centerRightOffset = centerOld - (lengthRight / 2.0f);

    Vec3i newBlocksLeft     = blocks;
    Vec3i newBlocksRight    = blocks;
    Vec3d newOffsetLeft     = offset;
    Vec3d newOffsetRight    = offset;
    Vec3d newPositionLeft   = position;
    Vec3d newPositionRight  = position;

    newBlocksLeft[divDim]   = blocksLeft;
    newBlocksRight[divDim]  = blocksRight;
    newOffsetLeft[divDim]   += centerLeftOffset;
    newOffsetRight[divDim]  += centerRightOffset;
    newPositionLeft[divDim] += centerLeftOffset;
    newPositionRight[divDim] += centerRightOffset;
    Vec3d newSizeLeft       = Vec3d(newBlocksLeft) * workpiece.blockSize;
    Vec3d newSizeRight      = Vec3d(newBlocksRight) * workpiece.blockSize;

    VRWorkpieceElement* left = new VRWorkpieceElement(workpiece, this, newBlocksLeft,
        newSizeLeft, newOffsetLeft, newPositionLeft, level + 1);
    VRWorkpieceElement* right = new VRWorkpieceElement(workpiece, this, newBlocksRight,
        newSizeRight, newOffsetRight, newPositionRight, level + 1);

    children[0] = left;
    children[1] = right;
}

bool VRWorkpieceElement::collides(Vec3d position) {
    Vec3d elementSize = this->size;
    Vec3d distance = this->position - position;
    Vec3d collisionDistance = (elementSize / 2.0f) + Vec3d(0.3f, 0.3f, 0.3f);

    if (abs(distance[0]) <= collisionDistance[0]&&
        abs(distance[1]) <= collisionDistance[1]&&
        abs(distance[2]) <= collisionDistance[2]) {
            return true;
    }

    return false;
}

bool VRWorkpieceElement::collide(Vec3d position) {
    if (deleted) return false;

    bool result = false;

    if (doesCollide(position)) {
        if (children[0] == nullptr) {
            split();
        }

        if (children[0] != nullptr) {
            bool childrenDeleted = true;
            for (int i = 0; i < 2; i++) {
                result = children[i]->collide(position) | result;
                childrenDeleted = childrenDeleted & children[i]->deleted;
            }

            deleted = childrenDeleted;

            if (deleted) {
                deleteChildren();
            }
        }

        else {
            deleteElement();
            result = true;
        }
    }
    return result;
}

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

//Add of Marie
bool VRWorkpieceElement::doesCollide(Vec3d position) {
    //position in argument is the lowest part of the worktool and the (0,0) point of the profile

    float py = this->position[1], px = this->position[0], pz = this->position[2];
    float sy = this->size[1], sx = this->size[0], sz = this->size[2];
    float ptooly = position[1], ptoolx = position[0], ptoolz = position[2];

    float profileLength = workpiece.cuttingProfile->getLength();

    if ((py + sy/2.0f > ptooly) && (py - sy/2.0f < ptooly + profileLength))
    {
        float maximum = workpiece.cuttingProfile->maxProfile(position, this->position, this->size);

        if ((ptoolz - maximum < pz + sz/2.0f) && (ptoolz + maximum > pz - sz/2.0) &&
            (ptoolx - maximum < px + sx/2.0f) && (ptoolx + maximum > px - sx/2.0))
        {
            float diffx = ptoolx - px, diffz = ptoolz - pz;
            if (std::abs(diffx) <= sx / 2.0f) return true;
            if (std::abs(diffz) <= sz / 2.0f) return true;

            float sgnx = sgn(diffx), sgnz = sgn(diffz);
            Vec2d vertex(px + sgnx * sx / 2.0f, pz + sgnz * sz / 2.0f);
            Vec2d toolMid(ptoolx, ptoolz);
            Vec2d distanceVertexTool = vertex - toolMid;
            if (distanceVertexTool.length() < maximum) {
                return true;
            }
        }
    }

    return false;
}

void VRWorkpieceElement::deleteElement() {
    deleted = true;
    issueGeometryUpdate();
}

bool VRWorkpieceElement::isDeleted() const {
    return this->deleted;
}

void VRWorkpieceElement::build() {
    if (children[0] != nullptr &&
            level < workpiece.geometryCreateLevel) {
        deleteGeometry();
        children[0]->build();
        children[1]->build();
    }

    if (!updateIssued) {
        return;
    }

    if (geometry == nullptr) {
        geometry = VRGeometry::create("wpelem");
        geometry->setType(GL_QUADS);
        geometry->setMaterial(workpiece.getMaterial());
        workpiece.addChild(geometry);
    }

    GeoPnt3fPropertyRecPtr positions = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr normals = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr indices = GeoUInt32Property::create();

    uint32_t index = 0;
    build(positions, normals, indices, index);

    geometry->setPositions(positions);
    geometry->setNormals(normals);
    geometry->setIndices(indices, true);
    geometry->setPositionalTexCoords();

    updateIssued = false;
}

void VRWorkpieceElement::build(GeoPnt3fPropertyRecPtr positions,
                               GeoVec3fPropertyRecPtr normals,
                               GeoUInt32PropertyRecPtr indices,
                               uint32_t& index) {
    if (deleted) return;

    if (children[0] != nullptr && children[1] != nullptr) {
        children[0]->build(positions, normals, indices, index);
        children[1]->build(positions, normals, indices, index);
    }

    else {
        buildGeometry(positions, normals, indices, index);
    }
}

Vec3d VRWorkpieceElement::mulVec3f(Vec3d lhs, Vec3d rhs) {
    return Vec3d(lhs[0] * rhs[0], lhs[1] * rhs[1], lhs[2] * rhs[2]);
}

void VRWorkpieceElement::buildGeometry(GeoPnt3fPropertyRecPtr positions,
                                       GeoVec3fPropertyRecPtr normals,
                                       GeoUInt32PropertyRecPtr indices,
                                       uint32_t& index) {
    Vec3d planeDistance = this->size / 2.0f;
    // for each dimension
    for (int sgnIndex = 0; sgnIndex < 2; sgnIndex++) {
        for (int dimension = 0; dimension < 3; dimension++) {
            Vec3d planeNormal = planeOffsetMasks[0][dimension];
            Vec3d planeOffset = mulVec3f(planeOffsetMasks[sgnIndex][dimension], planeDistance);
            Vec3d planeMid = this->offset + planeOffset;
            for (int vertexNum = 0; vertexNum < 4; vertexNum++) {
                Vec3d vertexOffsetMask = vertexOffsetMasks[sgnIndex][dimension][vertexNum];
                Vec3d vertexOffset = mulVec3f(planeDistance, vertexOffsetMask);
                Pnt3d vertexPosition = planeMid + vertexOffset;
                positions->push_back(vertexPosition);
                normals->push_back(planeNormal);
                indices->push_back(index);
                index++;
            }
        }
    }
}

const Vec3d VRWorkpieceElement::planeOffsetMasks[2][3] = {
    { Vec3d(1.0f, 0.0f, 0.0f), Vec3d{0.0f, 1.0f, 0.0f}, Vec3d(0.0f, 0.0f, 1.0f) },
    { Vec3d(-1.0f, 0.0f, 0.0f), Vec3d{0.0f, -1.0f, 0.0f}, Vec3d(0.0f, 0.0f, -1.0f)}
};

const Vec3d VRWorkpieceElement::vertexOffsetMasks[2][3][4] = {
    // sign, dimension, vertexnumber
    {
        { // dimension 1,0,0
            Vec3d(0.0f, 1.0f, 1.0f),
            Vec3d(0.0f, -1.0f, 1.0f),
            Vec3d(0.0f, -1.0f, -1.0f),
            Vec3d(0.0f, 1.0f, -1.0f)
        },
        { // dimension 0,1,0
            Vec3d(1.0f, 0.0f, 1.0f),
            Vec3d(1.0f, 0.0f, -1.0f),
            Vec3d(-1.0f, 0.0f, -1.0f),
            Vec3d(-1.0f, 0.0f, 1.0f)
        },
        { // dimension 0,0,1
            Vec3d(1.0f, 1.0f, 0.0f),
            Vec3d(-1.0f, 1.0f, 0.0f),
            Vec3d(-1.0f, -1.0f, 0.0f),
            Vec3d(1.0f, -1.0f, 0.0f)
        }
    },
    // sign -1
    {
        { // dimension -1,0,0
            Vec3d(0.0f, -1.0f, -1.0f),
            Vec3d(0.0f, 1.0f, -1.0f),
            Vec3d(0.0f, 1.0f, 1.0f),
            Vec3d(0.0f, -1.0f, 1.0f)
        },
        { // dimension 0,-1,0
            Vec3d(-1.0f, 0.0f, -1.0f),
            Vec3d(-1.0f, 0.0f, 1.0f),
            Vec3d(1.0f, 0.0f, 1.0f),
            Vec3d(1.0f, 0.0f, -1.0f)
        },
        { // dimension 0,0,-1
            Vec3d(-1.0f, -1.0f, 0.0f),
            Vec3d(1.0f, -1.0f, 0.0f),
            Vec3d(1.0f, 1.0f, 0.0f),
            Vec3d(-1.0f, 1.0f, 0.0f)
        }
    }
};

const float VRWorkpieceElement::sign[2] = {1.0f, -1.0f};

