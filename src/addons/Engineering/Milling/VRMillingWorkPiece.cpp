#include "VRMillingWorkPiece.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"

#include <boost/bind.hpp>

OSG_BEGIN_NAMESPACE
using namespace std;

VRMillingWorkPiece::VRMillingWorkPiece(string name) : VRGeometry(name), rootElement(nullptr) {
	type = "MillingWorkPiece";
	uFkt = VRFunction<int>::create("MillingWorkPiece-update", boost::bind(&VRMillingWorkPiece::update, this));
	VRSceneManager::getCurrent()->addUpdateFkt(uFkt);
}

VRMillingWorkPiecePtr VRMillingWorkPiece::ptr() { return static_pointer_cast<VRMillingWorkPiece>( shared_from_this() ); }
VRMillingWorkPiecePtr VRMillingWorkPiece::create(string name) { return shared_ptr<VRMillingWorkPiece>(new VRMillingWorkPiece(name) ); }

void VRMillingWorkPiece::setCuttingTool(VRTransformPtr geo) {
    tool = geo;
    toolPose = geo->getWorldPose();
    lastToolChange = geo->getLastChange();


}

void VRMillingWorkPiece::init(Vec3i gSize, float bSize) {
    //printf("entered init function\n");
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
                                  Vec3i(gridSize), Vec3f(gridSize) * blockSize,
                                  Vec3f(0, 0, 0), getFrom(), 0);

    rootElement->build();
    //printf("ended init function\n");
}

void VRMillingWorkPiece::reset() {
    //printf("entered reset function\n");
    init(gridSize, blockSize);
    //printf("ended reset function\n");
}

void VRMillingWorkPiece::addPointProfile(Vec2f point) {
    //printf("entered addpointprofile function\n");
    this->profile.push_back(point);
    //printf("ended addpointprofile function\n");
}

void VRMillingWorkPiece::update() {
    //printf("entered update function\n");
    auto geo = tool.lock();
    if (!geo) return;
    int change = geo->getLastChange();
    if (change == lastToolChange) return; // keine bewegung
    lastToolChange = change;

    if (profile.size() == 0)
    {
        printf("Profile empty. Return.\n");
        return;
    }

    Vec3f toolPosition = geo->getWorldPosition();
    //printf("update function: before collide\n");
    if (!rootElement->collide(toolPosition))
    {
        printf("return\n");
        return;
    }
    //printf("update function: after collide\n");

    //printf("update function: before update geometry\n");
    if (updateCount++ % geometryUpdateWait == 0) {
        rootElement->build();
    }
    //printf("ended update function\n");
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
                                       Vec3i blocks, Vec3f size, Vec3f offset, Vec3f position, const int level)
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
    Vec3f newOffsetLeft     = offset;
    Vec3f newOffsetRight    = offset;
    Vec3f newPositionLeft   = position;
    Vec3f newPositionRight  = position;

    newBlocksLeft[divDim]   = blocksLeft;
    newBlocksRight[divDim]  = blocksRight;
    newOffsetLeft[divDim]   += centerLeftOffset;
    newOffsetRight[divDim]  += centerRightOffset;
    newPositionLeft[divDim] += centerLeftOffset;
    newPositionRight[divDim] += centerRightOffset;
    Vec3f newSizeLeft       = Vec3f(newBlocksLeft) * workpiece.blockSize;
    Vec3f newSizeRight      = Vec3f(newBlocksRight) * workpiece.blockSize;

    VRWorkpieceElement* left = new VRWorkpieceElement(workpiece, this, newBlocksLeft,
        newSizeLeft, newOffsetLeft, newPositionLeft, level + 1);
    VRWorkpieceElement* right = new VRWorkpieceElement(workpiece, this, newBlocksRight,
        newSizeRight, newOffsetRight, newPositionRight, level + 1);

    children[0] = left;
    children[1] = right;
}

bool VRWorkpieceElement::collides(Vec3f position) {
    Vec3f elementSize = this->size;
    Vec3f distance = this->position - position;
    Vec3f collisionDistance = (elementSize / 2.0f) + Vec3f(0.3f, 0.3f, 0.3f);

    if (abs(distance[0]) <= collisionDistance[0]&&
        abs(distance[1]) <= collisionDistance[1]&&
        abs(distance[2]) <= collisionDistance[2]) {
            return true;
    }

    return false;
}

bool VRWorkpieceElement::collide(Vec3f position) {
    //printf("entered collide function\n");
    if (deleted) return false;

    bool result = false;

    //printf("before doesCollide condition\n");
    //if (collides(position)) {
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
    //printf("ended collide function\n");
    return result;
}

//Add of Marie
bool VRWorkpieceElement::doesCollide(Vec3f position) {
    //printf("entered doescollide function\n");

    //position in argument is the lowest part of the worktool and the (0,0) point of the profile
    float py = this->position[1], px = this->position[0], pz = this->position[2];
    float sy = this->size[1], sx = this->size[0], sz = this->size[2];
    float ptooly = position[1], ptoolx = position[0], ptoolz = position[2];

    //printf("%i\n",workpiece.profile.size());
    float profileLength = workpiece.profile.back()[0];
    //printf("after test variable\n");
    if ((py + sy/2.0f > ptooly) && (py - sy/2.0f < ptooly + profileLength))
    {
        //printf("before maxprofile function\n");
        float maximum = maxProfile(position);
        printf("maximum: %f\n", maximum);
        printf("px: %f\n", px);
        printf("pz: %f\n", pz);
        printf("sx: %f\n", sx);
        printf("sz: %f\n", sz);
        printf("ptoolx: %f\n", ptoolx);
        printf("ptoolz: %f\n", ptoolz);
        //printf("before second if\n");
        //if ((abs(px - sx/2.0f - ptoolx) < abs(maximum - ptoolx))
        //    && (abs(pz - sz/2.0f - ptoolz) < abs(maximum - ptoolz)))
        if ((ptoolz - maximum < pz + sz/2.0f) && (ptoolz + maximum > pz - sz/2.0) && (ptoolx - maximum < px + sx/2.0f) && (ptoolx + maximum > px - sx/2.0))
        {
            //if ()
            printf("cut\n\n");
            return true;
        }
    }
    //printf("after first if\n");
    return false;
}

//Add of Marie
float VRWorkpieceElement::maxProfile(Vec3f position) {
    float py = this->position[1], px = this->position[0], pz = this->position[2];
    float sy = this->size[1], sx = this->size[0], sz = this->size[2];
    float ptooly = position[1], ptoolx = position[0], ptoolz = position[2];

    float newx1 = py + sy/2.0f - ptooly;
    float newx2 = py - sy/2.0f - ptooly;

    int indexMax = -1;
    int indexMin = -1;
    vector<Vec2f> newList;

    if ((newx1 > 0) && (newx2 < workpiece.profile.back()[0]))
    {
        int p1 = lookForNearestIndex(newx1);
        Vec2f newPoint1 = {newx1, newy(newx1, p1)};

        int p2 = lookForNearestIndex(newx2);
        Vec2f newPoint2 = {newx2, newy(newx2, p2)};

        if (newx1 < workpiece.profile[p1][0])
            indexMax = p1 - 1;
        else
            indexMax = p1;
        if (newx2 < workpiece.profile[p2][0])
            indexMin = p2 + 1;
        else
            indexMin = p2;

        newList.push_back(newPoint1);
        for (int i = indexMin; i <= indexMax; i++)
            newList.push_back(workpiece.profile[i]);
        newList.push_back(newPoint2);
    }
    else if (newx1 > 0)
    {
        int p = lookForNearestIndex(newx1);
        Vec2f newPoint = {newx1, newy(newx1, p)};

        if (newx1 < workpiece.profile[p][0])
            indexMax = p - 1;
        else
            indexMax = p;

        for (int i = 0; i <= indexMax; i++)
            newList.push_back(workpiece.profile[i]);
        newList.push_back(newPoint);
    }
    else if (newx2 < workpiece.profile.back()[0])
    {
        int p = lookForNearestIndex(newx2);
        Vec2f newPoint = {newx2, newy(newx2, p)};

        if (newx2 < workpiece.profile[p][0])
            indexMin = p + 1;
        else
            indexMin = p;

        newList.push_back(newPoint);
        for (int i = indexMin; i <= workpiece.profile.size()-1; i++)
            newList.push_back(workpiece.profile[i]);
    }

    return lookForMaxInList(newList);
}

//Add of Marie
float VRWorkpieceElement::newy(float newx, int p) {
    float a = 0, b = 0;

    if (newx < workpiece.profile[p][0])
    {
        a = (workpiece.profile[p-1][1] - workpiece.profile[p][1]) / (workpiece.profile[p-1][0] - workpiece.profile[p][0]);
        b = (workpiece.profile[p-1][0] * workpiece.profile[p][1] - workpiece.profile[p][0] * workpiece.profile[p-1][1]) / (workpiece.profile[p-1][0] - workpiece.profile[p][0]);
    }
    else
    {
        a = (workpiece.profile[p][1] - workpiece.profile[p+1][1])  / (workpiece.profile[p][0] - workpiece.profile[p+1][0]);
        b = (workpiece.profile[p][0] * workpiece.profile[p+1][1] - workpiece.profile[p+1][0] * workpiece.profile[p][1]) / (workpiece.profile[p][0] - workpiece.profile[p+1][0]);
    }

    return (a * newx + b);
}

//Add of Marie
int VRWorkpieceElement::lookForNearestIndex(float newx) {
    int index = 0;

    for(int i = 0; i < workpiece.profile.size()-1; i++)
    {
        if (abs(newx - workpiece.profile[i][0]) < abs(newx - workpiece.profile[index][0]))
        {
            index = i;
        }
    }

    return index;
}

//Add of Marie
float VRWorkpieceElement::lookForMaxInList(vector<Vec2f> liste) {
    float maximum = 0;

    for (int i = 0; i < liste.size()-1; i++)
    {
        if (liste[i][1] > maximum)
        {
            maximum = liste[i][1];
        }
    }

    return maximum;
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
    geometry->setIndices(indices);
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

Vec3f VRWorkpieceElement::mulVec3f(Vec3f lhs, Vec3f rhs) {
    return Vec3f(lhs[0] * rhs[0], lhs[1] * rhs[1], lhs[2] * rhs[2]);
}

void VRWorkpieceElement::buildGeometry(GeoPnt3fPropertyRecPtr positions,
                                       GeoVec3fPropertyRecPtr normals,
                                       GeoUInt32PropertyRecPtr indices,
                                       uint32_t& index) {
    Vec3f planeDistance = this->size / 2.0f;
    // for each dimension
    for (int sgnIndex = 0; sgnIndex < 2; sgnIndex++) {
        for (int dimension = 0; dimension < 3; dimension++) {
            Vec3f planeNormal = planeOffsetMasks[0][dimension];
            Vec3f planeOffset = mulVec3f(planeOffsetMasks[sgnIndex][dimension], planeDistance);
            Vec3f planeMid = this->offset + planeOffset;
            for (int vertexNum = 0; vertexNum < 4; vertexNum++) {
                Vec3f vertexOffsetMask = vertexOffsetMasks[sgnIndex][dimension][vertexNum];
                Vec3f vertexOffset = mulVec3f(planeDistance, vertexOffsetMask);
                Pnt3f vertexPosition = planeMid + vertexOffset;
                positions->push_back(vertexPosition);
                normals->push_back(planeNormal);
                indices->push_back(index);
                index++;
            }
        }
    }
}

const Vec3f VRWorkpieceElement::planeOffsetMasks[2][3] = {
    { Vec3f(1.0f, 0.0f, 0.0f), Vec3f{0.0f, 1.0f, 0.0f}, Vec3f(0.0f, 0.0f, 1.0f) },
    { Vec3f(-1.0f, 0.0f, 0.0f), Vec3f{0.0f, -1.0f, 0.0f}, Vec3f(0.0f, 0.0f, -1.0f)}
};

const Vec3f VRWorkpieceElement::vertexOffsetMasks[2][3][4] = {
    // sign, dimension, vertexnumber
    {
        { // dimension 1,0,0
            Vec3f(0.0f, 1.0f, 1.0f),
            Vec3f(0.0f, -1.0f, 1.0f),
            Vec3f(0.0f, -1.0f, -1.0f),
            Vec3f(0.0f, 1.0f, -1.0f)
        },
        { // dimension 0,1,0
            Vec3f(1.0f, 0.0f, 1.0f),
            Vec3f(1.0f, 0.0f, -1.0f),
            Vec3f(-1.0f, 0.0f, -1.0f),
            Vec3f(-1.0f, 0.0f, 1.0f)
        },
        { // dimension 0,0,1
            Vec3f(1.0f, 1.0f, 0.0f),
            Vec3f(-1.0f, 1.0f, 0.0f),
            Vec3f(-1.0f, -1.0f, 0.0f),
            Vec3f(1.0f, -1.0f, 0.0f)
        }
    },
    // sign -1
    {
        { // dimension -1,0,0
            Vec3f(0.0f, -1.0f, -1.0f),
            Vec3f(0.0f, 1.0f, -1.0f),
            Vec3f(0.0f, 1.0f, 1.0f),
            Vec3f(0.0f, -1.0f, 1.0f)
        },
        { // dimension 0,-1,0
            Vec3f(-1.0f, 0.0f, -1.0f),
            Vec3f(-1.0f, 0.0f, 1.0f),
            Vec3f(1.0f, 0.0f, 1.0f),
            Vec3f(1.0f, 0.0f, -1.0f)
        },
        { // dimension 0,0,-1
            Vec3f(-1.0f, -1.0f, 0.0f),
            Vec3f(1.0f, -1.0f, 0.0f),
            Vec3f(1.0f, 1.0f, 0.0f),
            Vec3f(-1.0f, 1.0f, 0.0f)
        }
    }
};

const float VRWorkpieceElement::sign[2] = {1.0f, -1.0f};

OSG_END_NAMESPACE
