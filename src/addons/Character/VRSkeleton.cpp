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

void VRSkeleton::resolveKinematics() {
    if (fabrik) fabrik->iterate();
    updateGeometry();
}

void VRSkeleton::updateGeometry() {
    fabrik->visualize(ptr());
    /*VRGeoData geo;
    asGeometry(geo);
    geo.apply( ptr() );*/
}

void VRSkeleton::setupSimpleHumanoid() {
    clear();

    int coreID = addJoint("core", Pose::create(Vec3d(0,1.2,0), Vec3d(0,1,0), Vec3d(0,0,1)));

	auto addLeg = [&](string n, float w, int i) {
		int hipID = addJoint("hip", Pose::create(Vec3d(w,1.0,0), Vec3d(0,1,0), Vec3d(0,0,1)));
		int kneeID = addJoint("knee", Pose::create(Vec3d(w,0.5,0), Vec3d(0,1,0), Vec3d(0,0,1)));
		int ankleID = addJoint("ankle", Pose::create(Vec3d(w,0,0), Vec3d(0,1,0), Vec3d(0,0,1)));
		int toesID = addJoint("toes", Pose::create(Vec3d(w,-0.2,0), Vec3d(0,1,0), Vec3d(0,0,1)));

		addChain(n+"_foot", {ankleID,toesID});
		addChain(n, {hipID,kneeID,ankleID});

		float a1 = 0.4; // 0.1
		float a2 = 2.0;
		fabrik->addConstraint(kneeID, Vec4d(a1,0,a1,a2)); // knee
		fabrik->addConstraint(ankleID, Vec4d(a1,a2,a1,a1)); // ankle

		fabrik->setTarget(ankleID, Pose::create(Vec3d(w,0,0)));
		fabrik->setTarget(toesID, Pose::create(Vec3d(w,-0.01,0.2)));
    };

	auto addArm = [&](string n, float w, int i) {
		int shoulderID = addJoint("shoulder", Pose::create(Vec3d(w,1.5,0)));
		int elbowID = addJoint("elbow", Pose::create(Vec3d(w,1.2,0)));
		int wristID = addJoint("wrist", Pose::create(Vec3d(w,0.9,0)));
		int handID = addJoint("hand", Pose::create(Vec3d(w,0.8,0)));
		addChain(n, {0,shoulderID,elbowID,wristID,handID});
		fabrik->setTarget(handID, Pose::create(Vec3d(w,0.8,0)));
    };

	addLeg("leg1", 0.2, 0);
	addLeg("leg2",-0.2, 4);
	addArm("arm1", 0.3, 8);
	addArm("arm2",-0.3,12);

	int headID = addJoint("head", Pose::create(Vec3d(0,1.6,0)));
	int neckID = addJoint("neck", Pose::create(Vec3d(0,1.8,0)));
	addChain("head", {0,headID,neckID});
	fabrik->setTarget(neckID, Pose::create(Vec3d(0,1.8,0)));

	updateGeometry();
}




