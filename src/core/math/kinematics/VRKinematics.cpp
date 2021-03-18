#include "VRKinematics.h"
#include "VRConstraint.h"
#include "core/objects/VRTransform.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/toString.h"
#include "core/math/pose.h"

using namespace OSG;

/**
Kinematics structure

- Joints
    - 6 DoF Joints
        - Ball/Hinge Joint
        - Sliding Joint
**/



VRKinematics::VRKinematics()
{
    graph = Graph::create();
}

VRKinematics::~VRKinematics()
{

}

VRKinematicsPtr VRKinematics::create() { return VRKinematicsPtr(new VRKinematics() ); }


int VRKinematics::addJoint(int nID1, int nID2, VRConstraintPtr c)
{
    int eID = graph->connect(nID1, nID2);
    joints[eID] = Joint(eID, nID1, nID2, c);
    bodies[nID1].joints.push_back(eID);
    bodies[nID2].joints.push_back(eID);
    VRConstraintPtr c2 = VRConstraint::create();
    //bodies[nID1].obj->attach(bodies[nID2].obj, c, c2);
    auto P1 = bodies[nID1].obj->getPhysics();
    auto P2 = bodies[nID2].obj->getPhysics();
    if (P1 && P2) P1->setConstraint( P2, c, c2 );
    return eID;
}

int VRKinematics::addBody(VRTransformPtr obj, bool dynamic) {
    int nID = graph->addNode(obj->getPose());
    bodies[nID] = Body(nID, obj);
    physicalize(nID, dynamic);
    return nID;
}

VRTransformPtr VRKinematics::getTransform(int nID) { return bodies[nID].obj; }

int VRKinematics::addHinge(int nID1, int nID2, PosePtr d1, PosePtr d2, int axis, float minRange, float maxRange)
{
    VRConstraintPtr c = VRConstraint::create();
    c->setMinMax(axis, minRange, maxRange);
    c->setReferenceA(d1);
    c->setReferenceB(d2);
    return addJoint(nID1, nID2, c);
}

int VRKinematics::addBallJoint(int nID1, int nID2, PosePtr d1, PosePtr d2)
{
    VRConstraintPtr c = VRConstraint::create();
    vector<int> dofs{3,4,5};
    c->free(dofs);
    c->setReferenceA(d1);
    c->setReferenceB(d2);
    return addJoint(nID1, nID2, c);
}

int VRKinematics::addFixedJoint(int nID1, int nID2, PosePtr d1, PosePtr d2) {
    if (!d1 || !d2) return -1;
    VRConstraintPtr c = VRConstraint::create();
    c->setReferenceA(d1);
    c->setReferenceB(d2);
    return addJoint(nID1, nID2, c);
}

int VRKinematics::addCustomJoint(int nID1, int nID2, PosePtr d1, PosePtr d2, vector<int> dofs, vector<float> minRange, vector<float> maxRange)
{
    if (!(dofs.size() == minRange.size() || dofs.size() == maxRange.size())) return -1;
    VRConstraintPtr c = VRConstraint::create();
    for (uint i = 0; i < dofs.size(); i++) c->setMinMax(dofs[i], minRange[i], maxRange[i]);
    c->setReferenceA(d1);
    c->setReferenceB(d2);
    return addJoint(nID1, nID2, c);
}

GraphPtr VRKinematics::getGraph() {
    return graph;
}

void VRKinematics::setDynamic(int nID, bool dynamic) {
    bodies[nID].obj->getPhysics()->setDynamic(dynamic);
}

void VRKinematics::physicalize(int nID, bool dynamic) {
    float ld = 1;
    float ad = 1;
    bodies[nID].obj->setPhysicsActivationMode(4);
    bodies[nID].obj->setCollisionGroup({1});
    bodies[nID].obj->setCenterOfMass(Vec3d(0,0,0));
    bodies[nID].obj->setDamping(ld,ad);
    bodies[nID].obj->setGravity(Vec3d(0, 0, 0));
    bodies[nID].obj->physicalize(true, dynamic, "Convex");
    bodies[nID].obj->setCollisionMargin(0);
}

void VRKinematics::physicalizeAll(bool dynamic) {
    for (auto b : bodies) physicalize(b.first, dynamic);
}


void VRKinematics::clear() {
    graph->clear();
    bodies.clear();
    joints.clear();
}




