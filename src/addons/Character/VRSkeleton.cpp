#include "VRSkeleton.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/math/partitioning/graph.h"
#include "core/math/Eigendecomposition.h"
#include "core/math/equation.h"
#include "core/math/VRKabschAlgorithm.h"
#include "core/math/kinematics/VRFABRIK.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "addons/Algorithms/VRPathFinding.h"

#include <OpenSG/OSGQuaternion.h>

using namespace OSG;


VRSkeleton::Configuration::Configuration(string n) { setNameSpace("skConfig"); setName(n); }
VRSkeleton::Configuration::~Configuration() {}
VRSkeleton::ConfigurationPtr VRSkeleton::Configuration::create(string n) { return ConfigurationPtr(new Configuration(n) ); }

void VRSkeleton::Configuration::setPose(int i, Vec3d p) { joints[i] = p; }


VRSkeleton::VRSkeleton() : VRGeometry("skeleton") {
    fabrik = FABRIK::create();
}

VRSkeleton::~VRSkeleton() {}

VRSkeletonPtr VRSkeleton::create() { return VRSkeletonPtr(new VRSkeleton() ); }

FABRIKPtr VRSkeleton::getKinematics() { return fabrik; }

void VRSkeleton::clear() {
    joints.clear();
    fabrik = FABRIK::create();
}

int VRSkeleton::addJoint(string name, PosePtr p) {
    int jID = fabrik->size();
    fabrik->addJoint(jID, p);
    joints[name] = jID;
    return jID;
}

void VRSkeleton::addChain(string name, vector<int> jIDs) {
    fabrik->addChain(name, jIDs);
}

void VRSkeleton::addConstraint(string name, Vec4d angles) {
    if (!joints.count(name)) return;
    fabrik->addConstraint(joints[name], angles);
}

void VRSkeleton::addTarget(string name, PosePtr p) {
    if (!joints.count(name)) return;
    fabrik->setTarget(joints[name], p);
    targets[name] = p;
}

PosePtr VRSkeleton::getTarget(string name) {
    if (!targets.count(name)) return 0;
    return targets[name];
}

void VRSkeleton::resolveKinematics() {
    if (fabrik) fabrik->iterate();
    updateGeometry();
}

vector<VRSkeleton::Bone> VRSkeleton::getBones() {
    vector<VRSkeleton::Bone> res;

    for (auto c : fabrik->getChains()) {
        auto joints = fabrik->getChainJoints(c);
        for (int i=1; i<joints.size(); i++) {
            auto p1 = fabrik->getJointPose(joints[i-1]);
            auto p2 = fabrik->getJointPose(joints[i]);
            Bone b;
            b.ID = res.size();
            b.p1 = p1->pos();
            b.p2 = p2->pos();
            b.dir = b.p2-b.p1;
            b.dir.normalize();
            b.up = p1->up();
            b.up.normalize();
            b.length = (b.p2-b.p1).length();
            res.push_back(b);
        }
    }

    return res;
}

void VRSkeleton::setupSimpleHumanoid() {
    clear();

    int coreID = addJoint("core", Pose::create(Vec3d(0,1.2,0), Vec3d(0,1,0), Vec3d(0,0,1)));

	auto addLeg = [&](string side, float w, int i) {
		int hipID = addJoint("hip"+side, Pose::create(Vec3d(w,1.0,0), Vec3d(0,1,0), Vec3d(0,0,1)));
		int kneeID = addJoint("knee"+side, Pose::create(Vec3d(w,0.5,0), Vec3d(0,1,0), Vec3d(0,0,1)));
		int ankleID = addJoint("ankle"+side, Pose::create(Vec3d(w,0,0), Vec3d(0,1,0), Vec3d(0,0,1)));
		int toesID = addJoint("toes"+side, Pose::create(Vec3d(w,-0.2,0), Vec3d(0,1,0), Vec3d(0,0,1)));

		addChain("foot"+side, {ankleID,toesID});
		addChain("leg"+side, {hipID,kneeID,ankleID});

		float a1 = 0.4; // 0.1
		float a2 = 2.0;
		addConstraint("knee"+side, Vec4d(a1,0,a1,a2)); // knee
		addConstraint("ankle"+side, Vec4d(a1,a2,a1,a1)); // ankle

		addTarget("ankle"+side, Pose::create(Vec3d(w,0,0)));
		addTarget("toes"+side, Pose::create(Vec3d(w,-0.01,0.2)));
    };

	auto addArm = [&](string side, float w, int i) {
		int shoulderID = addJoint("shoulder"+side, Pose::create(Vec3d(w,1.5,0)));
		int elbowID = addJoint("elbow"+side, Pose::create(Vec3d(w,1.2,0)));
		int wristID = addJoint("wrist"+side, Pose::create(Vec3d(w,0.9,0)));
		int handID = addJoint("hand"+side, Pose::create(Vec3d(w,0.8,0)));
		addChain("arm"+side, {0,shoulderID,elbowID,wristID,handID});
		addTarget("hand"+side, Pose::create(Vec3d(w,0.8,0)));
    };

	addLeg("L", 0.2, 0);
	addLeg("R",-0.2, 4);
	addArm("L", 0.3, 8);
	addArm("R",-0.3,12);

	int headID = addJoint("head", Pose::create(Vec3d(0,1.6,0)));
	int neckID = addJoint("neck", Pose::create(Vec3d(0,1.8,0)));
	addChain("head", {0,headID,neckID});
	addTarget("neck", Pose::create(Vec3d(0,1.8,0)));

    fabrik->iterate();
	updateGeometry();
}

void VRSkeleton::updateGeometry() {
    fabrik->visualize(ptr());
    /*VRGeoData geo;
    asGeometry(geo);
    geo.apply( ptr() );*/
}

void VRSkeleton::asGeometry(VRGeoData& data) {
    Vec3d n(1,0,0);
    Color3f red(1,0,0);
    Color3f green(0,1,0);
    Color3f yellow(1,1,0);

    /*for (auto& b : bones) { // TODO
        auto& bone = b.second;
        Pnt3d p1 = bone.pose.pos() + bone.pose.dir() * bone.length*0.5;
        Pnt3d p2 = bone.pose.pos() - bone.pose.dir() * bone.length*0.5;
        Pnt3d p3 = bone.pose.pos() - bone.pose.dir() * bone.length*0.3 + bone.pose.up()*0.1*bone.length;
        Pnt3d p4 = bone.pose.pos() - bone.pose.dir() * bone.length*0.3 - bone.pose.up()*0.1*bone.length;
        Pnt3d p5 = bone.pose.pos() - bone.pose.dir() * bone.length*0.3 + bone.pose.x()*0.1*bone.length;
        Pnt3d p6 = bone.pose.pos() - bone.pose.dir() * bone.length*0.3 - bone.pose.x()*0.1*bone.length;
        int v1 = data.pushVert(p1, n, green);
        int v2 = data.pushVert(p2, n, green);
        int v3 = data.pushVert(p3, n, yellow);
        int v4 = data.pushVert(p4, n, green*0.5);
        int v5 = data.pushVert(p5, n, green);
        int v6 = data.pushVert(p6, n, green);
        data.pushLine(v1, v3);
        data.pushLine(v3, v2);
        data.pushLine(v1, v4);
        data.pushLine(v4, v2);
        data.pushLine(v1, v5);
        data.pushLine(v5, v2);
        data.pushLine(v1, v6);
        data.pushLine(v6, v2);
        data.pushLine(v3, v5);
        data.pushLine(v5, v4);
        data.pushLine(v4, v6);
        data.pushLine(v6, v3);
        //cout << "create bone geo " << bone.length << "  " << bone.pose.pos() << endl;
    }*/
}

void VRSkeleton::setupGeometry() {
    auto mS = VRMaterial::get("skeleton");
    mS->setLit(0);
    mS->setLineWidth(2);
    mS->setPointSize(4);
    setMaterial(mS);
	updateGeometry();
}





