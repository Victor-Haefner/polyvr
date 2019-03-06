#include "VRSkeleton.h"
#include "core/utils/toString.h"
#include "core/math/graph.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"


using namespace OSG;

template<> string typeName(const VRSkeleton& m) { return "Skeleton"; }


VRSkeleton::Configuration::Configuration(string n) { setNameSpace("skConfig"); setName(n); }
VRSkeleton::Configuration::~Configuration() {}
VRSkeleton::ConfigurationPtr VRSkeleton::Configuration::create(string n) { return ConfigurationPtr(new Configuration(n) ); }

void VRSkeleton::Configuration::setPose(int i, Vec3d p) { joints[i] = p; }


VRSkeleton::VRSkeleton() {
    armature = Graph::create();
}

VRSkeleton::~VRSkeleton() {}

VRSkeletonPtr VRSkeleton::create() { return VRSkeletonPtr(new VRSkeleton() ); }

void VRSkeleton::clear() {
    bones.clear();
    joints.clear();
    endEffectors.clear();
    armature->clear();
    rootBone = -1;
}

int VRSkeleton::addBone(PosePtr pose, float length) {
    int nID = armature->addNode();
    bones[nID].pose = *pose;
    bones[nID].length = length;
    return nID;
}

int VRSkeleton::addJoint(int bone1, int bone2, VRConstraintPtr constraint) {
    int eID = armature->connect(bone1, bone2);
    joints[eID].bone1 = bone1;
    joints[eID].bone2 = bone2;
    joints[eID].constraint = constraint;
    return eID;
}

void VRSkeleton::setEndEffector(string label, int bone) { endEffectors[label] = bone; }
void VRSkeleton::setRootBone(int bone) { rootBone = bone; }

void VRSkeleton::asGeometry(VRGeoData& data) {
    Vec3d n(1,0,0);
    Color3f red(1,0,0);
    Color3f green(0,1,0);
    Color3f yellow(1,1,0);

    for (auto& b : bones) {
        auto& bone = b.second;
        Pnt3d p1 = bone.pose.pos() + bone.pose.dir() * bone.length*0.5;
        Pnt3d p2 = bone.pose.pos() - bone.pose.dir() * bone.length*0.5;
        int v1 = data.pushVert(p1, n, green);
        int v2 = data.pushVert(p2, n, green);
        data.pushLine(v1, v2);
        cout << "create bone geo " << bone.length << "  " << bone.pose.pos() << endl;
    }

    for (auto& j : joints) {
        auto& joint = j.second;
        auto& bone1 = bones[joint.bone1];
        auto& bone2 = bones[joint.bone2];
        Pnt3d p1 = bone1.pose.transform( joint.constraint->getReferenceA()->pos() + Vec3d(0,0.01,0) );
        Pnt3d p2 = bone2.pose.transform( joint.constraint->getReferenceB()->pos() - Vec3d(0,0.01,0) );
        int v1 = data.pushVert(p1, n, red);
        int v2 = data.pushVert(p2, n, yellow);
        data.pushPoint(v1);
        data.pushPoint(v2);

        Pnt3d b1 = bone1.pose.transform( Vec3d(0, 0.01,0) );
        Pnt3d b2 = bone2.pose.transform( Vec3d(0,-0.01,0) );
        int l1 = data.pushVert(b1, n, red);
        int l2 = data.pushVert(b2, n, yellow);
        data.pushLine(l1,v1);
        data.pushLine(l2,v2);
    }
}

void VRSkeleton::setupGeometry() {
    VRGeoData geo;
    asGeometry(geo);
    geo.apply( ptr() );

    auto m = VRMaterial::get("skeleton");
    m->setLit(0);
    m->setDiffuse(Color3f(0,1,0));
    m->setLineWidth(2);
    m->setPointSize(4);
    setMaterial(m);
}

void VRSkeleton::updateGeometry() {
    VRGeoData geo;
    asGeometry(geo);
    geo.apply( ptr() );

    /*VRGeoData data(ptr());
    auto& bones = armature->getNodes();
    for (uint i=0; i<joints.size(); i++) {
        auto& joint = joints[i];
        data.setVert(i, Pnt3d(joint.box.center()) );
    }*/
}

void VRSkeleton::setupSimpleHumanoid() {
    clear();

    auto ballJoint = [&](Vec3d offsetA, Vec3d offsetB) {
        auto joint = VRConstraint::create();
        for (int i=3; i<6; i++) joint->setMinMax(i, -Pi*0.5, Pi*0.5);
        joint->setReferenceA(Pose::create(offsetA));
        joint->setReferenceB(Pose::create(offsetB));
        return joint;
    };

    auto hingeJoint = [&](Vec3d offsetA, Vec3d offsetB) {
        auto joint = VRConstraint::create();
        joint->setMinMax(5, 0, Pi*0.5);
        joint->setReferenceA(Pose::create(offsetA));
        joint->setReferenceB(Pose::create(offsetB));
        return joint;
    };

    // spine
    auto waist = ballJoint(Vec3d(0,0,0.15), Vec3d(0,0,-0.2));
    auto neck  = ballJoint(Vec3d(0,0,0.2), Vec3d(0,0,-0.1));
    int abdomen = addBone(Pose::create(Vec3d(0,1.15,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.3);
    int back    = addBone(Pose::create(Vec3d(0,1.5,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.4);
    int head    = addBone(Pose::create(Vec3d(0,1.8,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.2);
    addJoint(abdomen, back, waist);
    addJoint(back, head, neck);

    // legs
    auto ankle = ballJoint(Vec3d(0,0,0.1), Vec3d(0,0,-0.25));
    auto knee  = hingeJoint(Vec3d(0,0,0.25), Vec3d(0,0,-0.25));
    for (auto i : {-0.25,0.25}) {
        auto hip = ballJoint(Vec3d(0,0,0.25), Vec3d(i,0,-0.15));
        int foot     = addBone(Pose::create(Vec3d(i,0,-0.1),Vec3d(0,0,-1),Vec3d(0,1,0)), 0.2);
        int lowerLeg = addBone(Pose::create(Vec3d(i,0.25,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.5);
        int upperLeg = addBone(Pose::create(Vec3d(i,0.75,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.5);
        addJoint(foot, lowerLeg, ankle);
        addJoint(lowerLeg, upperLeg, knee);
        addJoint(upperLeg, abdomen, hip);
        if (i > 0) setEndEffector("footRight", foot);
        if (i < 0) setEndEffector("footLeft", foot);
    }

    // arms
    auto wrist = ballJoint(Vec3d(0,0,0.05), Vec3d(0,0,-0.15));
    auto elbow = hingeJoint(Vec3d(0,0,0.15), Vec3d(0,0,-0.15));
    for (auto i : {-0.2,0.2}) {
        auto shoulder = ballJoint(Vec3d(0,0,0.15), Vec3d(i,0,0.2));
        int hand     = addBone(Pose::create(Vec3d(i,1.05,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.1);
        int lowerArm = addBone(Pose::create(Vec3d(i,1.25,0) ,Vec3d(0,-1,0),Vec3d(0,0,1)), 0.3);
        int upperArm = addBone(Pose::create(Vec3d(i,1.55,0) ,Vec3d(0,-1,0),Vec3d(0,0,1)), 0.3);
        addJoint(hand, lowerArm, wrist);
        addJoint(lowerArm, upperArm, elbow);
        addJoint(upperArm, back, shoulder);
        if (i > 0) setEndEffector("handRight", hand);
        if (i < 0) setEndEffector("handLeft", hand);
    }

    setRootBone(abdomen);
}

void VRSkeleton::move(string endEffector, PosePtr pose) {
    auto& bone = bones[ endEffectors[endEffector] ];
    bone.pose = *pose;
    updateGeometry();
}





