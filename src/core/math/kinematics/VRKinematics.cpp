#include "VRKinematics.h"
#include "VRConstraint.h"
#include "core/objects/VRTransform.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/toString.h"
#include "core/math/pose.h"

using namespace OSG;
template<> string typeName(const VRKinematics& k) { return "VRKinematics"; }
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
    return eID;
}

int VRKinematics::addBody(VRTransformPtr obj)
{
    int nID = graph->addNode(obj->getPose());
    bodies[nID] = Body(nID, obj);
    return nID;

}

int VRKinematics::setupHinge(int nID1, int nID2, PosePtr d1, PosePtr d2, int axis, float minRange, float maxRange)
{
    VRConstraintPtr c = VRConstraint::create();
    c->setMinMax(axis, minRange, maxRange);
    c->setReferenceA(d1);
    c->setReferenceB(d2);

    VRConstraintPtr c2 = VRConstraint::create();
    bodies[nID1].obj->attach(bodies[nID2].obj, c, c2);

    return addJoint(nID1, nID2, c);
}

int VRKinematics::setupBallJoint(int nID1, int nID2, PosePtr d1, PosePtr d2)
{
    VRConstraintPtr c = VRConstraint::create();
    vector<int> dof;
    dof.push_back(3);
    dof.push_back(4);
    dof.push_back(5);

    c->free(dof);
    c->setReferenceA(d1);
    c->setReferenceB(d2);

    VRConstraintPtr c2 = VRConstraint::create();
    bodies[nID1].obj->attach(bodies[nID2].obj, c, c2);

    return addJoint(nID1, nID2, c);
}

int VRKinematics::setupFixedJoint(int nID1, int nID2, PosePtr d1, PosePtr d2)
{
    VRConstraintPtr c = VRConstraint::create();
    c->setReferenceA(d1);
    c->setReferenceB(d2);

    VRConstraintPtr c2 = VRConstraint::create();
    bodies[nID1].obj->attach(bodies[nID2].obj, c, c2);
    return addJoint(nID1, nID2, c);
}

int VRKinematics::setupCustomJoint(int nID1, int nID2, PosePtr d1, PosePtr d2, vector<int> dofs, vector<float> minRange, vector<float> maxRange)
{
return 0;
}

GraphPtr VRKinematics::getGraph() {
    return graph;
}
