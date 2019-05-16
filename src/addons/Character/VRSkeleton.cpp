#include "VRSkeleton.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/math/graph.h"
#include "core/math/Eigendecomposition.h"
#include "core/math/SingularValueDecomposition.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "addons/Algorithms/VRPathFinding.h"

#include <OpenSG/OSGQuaternion.h>

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

int VRSkeleton::addBone(PosePtr pose, float length, string name) {
    int nID = armature->addNode();
    bones[nID].pose = *pose;
    bones[nID].length = length;
    bones[nID].name = name;
    return nID;
}

int VRSkeleton::addJoint(int bone1, int bone2, VRConstraintPtr constraint, string name, Color3f col) {
    int eID = armature->connect(bone1, bone2);
    joints[eID].bone1 = bone1;
    joints[eID].bone2 = bone2;
    joints[eID].constraint = constraint;
    joints[eID].name = name;
    joints[eID].col = col;
    return eID;
}

void VRSkeleton::setEndEffector(string label, int bone) {
    if (!endEffectors.count(label)) endEffectors[label] = EndEffector();
    endEffectors[label].boneID = bone;
    endEffectors[label].jointID = armature->getInEdges( bone )[0].ID;
}

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
    auto mS = VRMaterial::get("skeleton");
    mS->setLit(0);
    mS->setLineWidth(2);
    mS->setPointSize(4);
    setMaterial(mS);

    jointsGeo = VRGeometry::create("joints");
    addChild(jointsGeo);
    auto mJ = VRMaterial::get("skeletonJoints");
    mJ->setLit(0);
    mJ->setPointSize(15);
    mJ->setZOffset(1,1);
	jointsGeo->setMaterial(mJ);

    constraintsGeo = VRGeometry::create("constraints");
    addChild(constraintsGeo);
    auto mC = VRMaterial::get("constraints");
    mC->setLit(0);
    mC->setLineWidth(2);
    mC->setWireFrame(1);
	constraintsGeo->setMaterial(mC);

	updateGeometry();
}

void VRSkeleton::updateGeometry() {
    VRGeoData geo;
    asGeometry(geo);
    geo.apply( ptr() );

    // joints
    VRGeoData geo2;
    for (auto j : joints) {
        geo2.pushVert(j.second.pos, Vec3d(0,1,0), j.second.col);
		geo2.pushPoint();
    }

    // endeffectors
    for (auto e : endEffectors) {
        if (!e.second.target) continue;
        auto& joint = joints[ e.second.jointID ];
        Vec3d p = e.second.target->transform( joint.constraint->getReferenceB()->pos() );
        geo2.pushVert(p, Vec3d(0,1,0), Color3f(1,0.5,0.2));
		geo2.pushPoint();
    }
    geo2.apply( jointsGeo );

    // TODO
    // constraints
    VRGeoData geo3;
    Color3f col(0.6,0.8,1.0);
    Color3f col2(1.0,0.8,0.5);

    for (auto j : joints) {
        Vec3d d = j.second.dir1;
        geo3.pushVert(j.second.pos, Vec3d(0,1,0), col);
        geo3.pushVert(j.second.pos + d*0.2, Vec3d(0,1,0), col);
        geo3.pushLine();

        d = j.second.dir2;
        geo3.pushVert(j.second.pos, Vec3d(0,1,0), col);
        geo3.pushVert(j.second.pos + d*0.2, Vec3d(0,1,0), col);
        geo3.pushLine();

        Vec3d u = j.second.up1;
        geo3.pushVert(j.second.pos, Vec3d(0,1,0), col2);
        geo3.pushVert(j.second.pos + u*0.2, Vec3d(0,1,0), col2);
        geo3.pushLine();

        u = j.second.up2;
        geo3.pushVert(j.second.pos, Vec3d(0,1,0), col2);
        geo3.pushVert(j.second.pos + u*0.2, Vec3d(0,1,0), col2);
        geo3.pushLine();
    }

    geo3.apply( constraintsGeo );

}

void VRSkeleton::setupSimpleHumanoid() {
    clear();

    auto ballJoint = [&](PosePtr offsetA, PosePtr offsetB) {
        auto joint = VRConstraint::create();
        for (int i=3; i<6; i++) joint->setMinMax(i, -Pi*0.5, Pi*0.5);
        joint->setReferenceA(offsetA);
        joint->setReferenceB(offsetB);
        return joint;
    };

    auto hingeJoint = [&](PosePtr offsetA, PosePtr offsetB) {
        auto joint = VRConstraint::create();
        joint->setMinMax(5, 0, Pi*0.5);
        joint->setReferenceA(offsetA);
        joint->setReferenceB(offsetB);
        return joint;
    };

    // spine
    auto waist = ballJoint(Pose::create(Vec3d(0,0,0.15), Vec3d(0,0,1)), Pose::create(Vec3d(0,0,-0.2)));
    auto neck  = ballJoint(Pose::create(Vec3d(0,0,0.2), Vec3d(0,0,1)), Pose::create(Vec3d(0,0,-0.1)));
    int abdomen = addBone(Pose::create(Vec3d(0,1.15,0),Vec3d(0,-1,0),Vec3d(0,0,-1)), 0.3, "abdomen");
    int back    = addBone(Pose::create(Vec3d(0,1.5,0),Vec3d(0,-1,0),Vec3d(0,0,-1)), 0.4, "back");
    int head    = addBone(Pose::create(Vec3d(0,1.8,0),Vec3d(0,-1,0),Vec3d(0,0,-1)), 0.2, "head");
    addJoint(abdomen, back, waist, "waist");
    addJoint(back, head, neck, "neck");
    setEndEffector("head", head);

    // legs
    auto ankle = ballJoint(Pose::create(Vec3d(0,0,-0.25)), Pose::create(Vec3d(0,0,0.1), Vec3d(0,0,1)));
    auto knee  = hingeJoint(Pose::create(Vec3d(0,0,-0.25)), Pose::create(Vec3d(0,0,0.25), Vec3d(0,0,1)));
    for (auto i : {-0.25,0.25}) {
        string side = i < 0 ? "Left" : "Right";
        Color3f sc = i < 0 ? Color3f(1,0,0) : Color3f(0,0,1);
        auto hip = ballJoint(Pose::create(Vec3d(i,0,-0.15), Vec3d(i*4,0,0)), Pose::create(Vec3d(0,0,0.25), Vec3d(0,0,1)));
        int foot     = addBone(Pose::create(Vec3d(i,0,-0.1),Vec3d(0,0,-1),Vec3d(0,1,0)), 0.2, "foot"+side);
        int lowerLeg = addBone(Pose::create(Vec3d(i,0.25,0),Vec3d(0,-1,0),Vec3d(0,0,-1)), 0.5, "lLeg"+side);
        int upperLeg = addBone(Pose::create(Vec3d(i,0.75,0),Vec3d(0,-1,0),Vec3d(0,0,-1)), 0.5, "uLeg"+side);
        addJoint(abdomen, upperLeg, hip, "hip"+side, sc);
        addJoint(upperLeg, lowerLeg, knee, "knee"+side, sc);
        addJoint(lowerLeg, foot, ankle, "ankle"+side, sc);
        setEndEffector("foot"+side, foot);
    }

    // arms
    auto wrist = ballJoint(Pose::create(Vec3d(0,0,-0.15)), Pose::create(Vec3d(0,0,0.05), Vec3d(0,0,1)));
    auto elbow = hingeJoint(Pose::create(Vec3d(0,0,-0.15)), Pose::create(Vec3d(0,0,0.15), Vec3d(0,0,1)));
    for (auto i : {-0.2,0.2}) {
        string side = i < 0 ? "Left" : "Right";
        Color3f sc = i < 0 ? Color3f(1,0,0) : Color3f(0,0,1);
        auto shoulder = ballJoint( Pose::create(Vec3d(i,0,0.2), Vec3d(i*5,0,0)), Pose::create(Vec3d(0,0,0.15), Vec3d(0,0,1)));
        int hand     = addBone(Pose::create(Vec3d(i,1.05,0),Vec3d(0,-1,0),Vec3d(0,0,-1)), 0.1, "hand"+side);
        int lowerArm = addBone(Pose::create(Vec3d(i,1.25,0) ,Vec3d(0,-1,0),Vec3d(0,0,-1)), 0.3, "lArm"+side);
        int upperArm = addBone(Pose::create(Vec3d(i,1.55,0) ,Vec3d(0,-1,0),Vec3d(0,0,-1)), 0.3, "uArm"+side);
        addJoint(back, upperArm, shoulder, "shoulder"+side, sc);
        addJoint(upperArm, lowerArm, elbow, "elbow"+side, sc);
        addJoint(lowerArm, hand, wrist, "wrist"+side, sc);
        setEndEffector("hand"+side, hand);
    }

    setRootBone(abdomen);
}

void VRSkeleton::updateJointPositions() {
    //cout << "VRSkeleton::updateJointPositions" << endl;
    for (auto& j : joints) {
        auto& bone1 = bones[j.second.bone1];
        auto& bone2 = bones[j.second.bone2];
        j.second.pos = bone1.pose.transform( j.second.constraint->getReferenceA()->pos() );
        j.second.dir1 = bone1.pose.transform( j.second.constraint->getReferenceA()->dir(), false );
        j.second.dir2 = bone2.pose.transform( j.second.constraint->getReferenceB()->dir(), false );
        j.second.up1  = bone1.pose.transform( j.second.constraint->getReferenceA()->up(), false );
        j.second.up2  = bone2.pose.transform( j.second.constraint->getReferenceB()->up(), false );
        //cout << " joint: " << j.second.name << ", bone1: " << bone1.name << ", jPos: " << j.second.pos << ", refA: " << j.second.constraint->getReferenceA()->pos() << endl;
    }
};

map<int, Vec3d> VRSkeleton::getJointsPositions() {
    map<int, Vec3d> res;
    for (auto j : joints) res[j.first] = j.second.pos;
    return res;
}

vector<int> VRSkeleton::getBoneJoints(int bone) {
    vector<int> res;
    auto edges = armature->getConnectedEdges( armature->getNode(bone) );
    for (auto e : edges) res.push_back(e.ID);
    return res;
}

Vec3d& VRSkeleton::jointPos(int j) { return joints[j].pos; };

void VRSkeleton::overrideSim(VRUpdateCbPtr cb) { simCB = cb; }

void VRSkeleton::applyFABRIK(string EE) {
    ChainData& data = ChainDataMap[EE];
    auto targetPos = data.targetPos;
    float tol = 0.001; // 1 mm tolerance
    vector<float>& distances = data.d;
    int Nd = distances.size();

    bool verbose = false;

    auto pQuat = [](Quaterniond& q) {
        double a; Vec3d d;
        q.getValueAsAxisRad(d,a);
        return "(" + toString(d) + ") " + toString(a);
    };

    auto sum = [](vector<float> v) {
        float r = 0;
        for (auto f : v) r += f;
        return r;
    };

    auto interp = [](Vec3d& a, Vec3d& b, float t) {
        return a*t + b*(1-t);
    };

    auto movePointTowards = [&](int i, Vec3d target, float t) -> Vec3d {
        auto& J = joints[data.joints[i]];
        Vec3d pOld = J.pos;
        J.pos = interp(J.pos, target, t);
        if (verbose) cout << " movePointTowards: " << J.name << " (" << pOld << ") -> " << 1-t << " / " << target << " -> " << J.pos << endl;
        return pOld;
    };

    auto checkDistance = [&](int i1, int i2, int id) -> Vec3d {
        auto& J1 = joints[data.joints[i1]];
        auto& J2 = joints[data.joints[i2]];
        if (verbose) cout << "checkDistance between: " << J1.name << " -> " << J2.name << endl;
        float li = distances[id] / (J2.pos - J1.pos).length();
        return movePointTowards(i1, J2.pos, li);
    };

    auto getRotation = [&](int i1, int i2, Vec3d pOld) -> Quaterniond {
        auto& J1 = joints[data.joints[i1]];
        auto& J2 = joints[data.joints[i2]];
        Vec3d d1 = pOld   - J1.pos;
        Vec3d d2 = J2.pos - J1.pos;
        d1.normalize();
        d2.normalize();
        auto q = Quaterniond(d1,d2);
        if (verbose) cout << " getRotation " << J1.name << " / " << J2.name << ", " << pQuat(q) << endl;
        if (verbose) cout << "      " << d1 << " / " << d2 << endl;
        return q;
    };

    auto rotateJoints = [&](int i1, int i2, Quaterniond& R) {
        auto& J1 = joints[data.joints[i1]];
        auto& J2 = joints[data.joints[i2]];
        R.multVec( J1.dir2, J1.dir2 );
        R.multVec( J1.up2, J1.up2 );
        R.multVec( J2.dir1, J2.dir1 );
        R.multVec( J2.up1, J2.up1 );
        if (verbose) cout << "   Rotate " << bones[J1.bone2].name << "  " << bones[J2.bone1].name << " " << pQuat(R) << endl << endl;
    };

    auto doBackAndForth = [&]() {
        for (int i = Nd-1; i > 0; i--) {
            auto pOld = checkDistance(i,i+1,i);
            if (i > 0) {
                auto R = getRotation(i-1, i, pOld);
                rotateJoints(i-1,i,R);
            }
        }

        for (int i = 1; i <= Nd; i++) {
            auto pOld = checkDistance(i,i-1,i-1);
            if (i < Nd) {
                auto R = getRotation(i+1, i, pOld);
                rotateJoints(i,i+1,R);
            }
        }
    };

    // basic FABRIK algorithm
    float Dtarget = (targetPos - jointPos(data.joints[0])).length();
    if (Dtarget > sum(distances)) { // position unreachable
        for (int i=1; i<=Nd; i++) {
            jointPos(data.joints[i]) = targetPos;
            checkDistance(i,i-1,i-1);
        }
    } else { // position reachable
        float difA = (jointPos(data.joints.back())-targetPos).length();
        int k=0;
        while (difA > tol) {
            k++; if(k>50) break;
            if (verbose) cout << "\n\nitr " << k << endl;

            int iE = data.joints.size()-1;
            Vec3d pOld = movePointTowards(iE, targetPos, 0);
            auto R = getRotation(iE-1, iE, pOld);
            rotateJoints(iE-1,iE,R);

            //jointPos(iE) = targetPos;
            doBackAndForth();
            difA = (jointPos(data.joints[iE])-targetPos).length();
            //break;
        }
    }
}

void VRSkeleton::resolveSystem(string bone) {
    auto& system = SystemDataMap[bone];
    //doBackAndForth(system);
    bool doContinue = true;
    while(doContinue) {
        doContinue = false;
        //cout << "b " << endl;
        for (auto j1 : system.joints) {
            //cout << " b " << j1 << endl;
            for (auto j2 : system.joints) {
                if (j2 == j1) continue;
                Vec3d M = (jointPos(j1) + jointPos(j2))*0.5;
                Vec3d D = jointPos(j2) - jointPos(j1);
                float d = system.d[j1][j2];
                float d2 = D.length();
                if (abs(d2-d) > 0.001) {
                    D.normalize();
                    D *= d*0.5;
                    jointPos(j1) = M - D;
                    jointPos(j2) = M + D;
                    doContinue = true;
                    //cout << "  b " << Vec2i(j1,j2) << "  " << d2 << " " << d << endl;
                }
            }
        }

        break;

        /*for (int i=1; i<system.joints.size(); i++) {
            int jID1 = system.joints[i-1];
            int jID2 = system.joints[i];
            Vec3d M = (jointPos(jID1) + jointPos(jID2))*0.5;
            Vec3d D = jointPos(jID2) - jointPos(jID1);
            float d = system.d[i-1];
            float d2 = D.length();
            if (abs(d2-d) > 0.001) {
                D.normalize();
                D *= d*0.5;
                jointPos(jID1) = M - D;
                jointPos(jID2) = M + D;
                doContinue = true;
            }
        }*/
    }
}

void VRSkeleton::simStep() {
    if (simCB) {
        (*simCB)();
        return;
    }

    cout << "simStep" << endl;
    applyFABRIK("handLeft");
    //applyFABRIK("handRight");

    //for (auto e : ChainDataMap) applyFABRIK(e.first);
    for (auto e : SystemDataMap) resolveSystem(e.first);
}

vector<int> VRSkeleton::getBonesChain(string endEffector) {
    int e = endEffectors[endEffector].boneID;
    VRPathFinding::Position pR(rootBone);
    VRPathFinding::Position pE(e);
    VRPathFinding pathFinding;
    pathFinding.setGraph(armature);
    auto path = pathFinding.computePath(pR, pE);
    vector<int> chainedBones;
    for (auto p : path) chainedBones.push_back(p.nID);
    return chainedBones;
};

vector<int> VRSkeleton::getJointsChain(vector<int>& chainedBones) {
    //cout << "VRSkeleton::getJointsChain:" << endl;
    vector<int> chainedJoints;
    for (int i=1; i<chainedBones.size(); i++) {
        int nID1 = chainedBones[i-1];
        int nID2 = chainedBones[i];
        int eID = armature->getEdgeID(nID1, nID2);
        chainedJoints.push_back(eID);
        //cout << " joint: " << joints[eID].name << " between " << bones[nID1].name << " and " << bones[nID2].name << endl;
    }
    return chainedJoints;
};

void VRSkeleton::resolveKinematics() {
    updateJointPositions();

    ChainDataMap.clear();
    SystemDataMap.clear();

    auto getChains = [&]() {
        for (auto& e : endEffectors) {
            ChainDataMap[e.first] = ChainData();
            ChainDataMap[e.first].chainedBones = getBonesChain(e.first);
            ChainDataMap[e.first].joints = getJointsChain(ChainDataMap[e.first].chainedBones);

            auto& joint = joints[ e.second.jointID ];
            auto pose = e.second.target;

            if (pose) ChainDataMap[e.first].targetPos = pose->transform( joint.constraint->getReferenceB()->pos() );
            else {
                ChainDataMap[e.first].targetPos = bones[joint.bone2].pose.transform( joint.constraint->getReferenceB()->pos() );
                e.second.target = Pose::create( bones[joint.bone2].pose );
            }

            for (int i=1; i<ChainDataMap[e.first].joints.size(); i++) {
                int jID1 = ChainDataMap[e.first].joints[i-1];
                int jID2 = ChainDataMap[e.first].joints[i];
                float d = (jointPos(jID1) - jointPos(jID2)).length();
                ChainDataMap[e.first].d.push_back( d );
            }
        }
    };

    auto getJointSystems = [&]() {
        for (auto& b : bones) {
            auto joints = getBoneJoints(b.first);
            if (joints.size() <= 2) continue; // not a system!

            auto& data = SystemDataMap[b.second.name];
            data.joints = joints;
            //data.joints.push_back(joints[0]); // close cycle
            data.bone = b.first;

            //cout << "a " << endl;
            for (auto j1 : data.joints) {
                //cout << " a " << j1 << endl;
                for (auto j2 : data.joints) {
                    if (j2 == j1) continue;
                    float d = (jointPos(j1) - jointPos(j2)).length();
                    data.d[j1][j2] = d;
                    //cout << "  a " << Vec2i(j1,j2) << "   " << d << " " << endl;
                }
            }

            /*for (int i=1; i<data.joints.size(); i++) {
                int jID1 = data.joints[i-1];
                int jID2 = data.joints[i];
                float d = (jointPos(jID1) - jointPos(jID2)).length();
                data.d.push_back( d );
            }*/
        }
    };

    getChains();
    getJointSystems();
    simStep();
    updateBones();
    updateGeometry();
}

class KabschAlgorithm {
    private:
        vector<Vec3d> points1;
        vector<Vec3d> points2;
        vector<Vec2i> matches;

        Vec3d centroid(vector<Vec3d> pnts) {
            Vec3d r;
            for (auto p : pnts) r += p;
            if (pnts.size() > 0) r /= pnts.size();
            return r;
        }

    public:
        KabschAlgorithm() {}

        void setPoints1( vector<Vec3d>& pnts ) { points1 = pnts; }
        void setPoints2( vector<Vec3d>& pnts ) { points2 = pnts; }
        void setMatches( vector<Vec2i>& mths ) { matches = mths; }

        void setSimpleMatches() {
            matches.clear();
            for (int i=0; i<points1.size(); i++) {
                matches.push_back(Vec2i(i,i));
            }
        }

        Matrix4d compute(bool verbose = false) {
            Vec3d c1 = centroid(points1);
            Vec3d c2 = centroid(points2);

            Matrix4d H, D, U, Ut, V, Vt, T;
            H.setScale(Vec3d(0,0,0));

            // covariance matrix H
            for (auto& m : matches) {
                Vec3d p1 = points1[m[0]] - c1;
                Vec3d p2 = points2[m[1]] - c2;
                for (int i=0; i<3; i++) {
                    for (int j=0; j<3; j++) {
                        H[i][j] += p1[i]*p2[j];
                    }
                }
            }

            SingularValueDecomposition svd(H, verbose);

            U = svd.U;
            V = svd.V;
            Ut.transposeFrom(svd.U);
            Vt.transposeFrom(svd.V);

            D = U;
            D.mult(Vt);
            float d = D.det(); // det( V Ut ) / det( U Vt )

            // R = V diag(1,1,d) Ut / R = U diag(1,1,d) Vt
            T.setScale(Vec3d(1,1,d));
            //T.setScale(Vec3d(d,d,d));
            if (verbose) {
                cout << "T\n" << T << endl;
                cout << "H\n" << H << endl;
                cout << "V\n" << V << endl;
                cout << "S\n" << svd.S << endl;
                cout << "U\n" << svd.U << endl;
                cout << "Ut\n" << Ut << endl;
                Matrix4d C = svd.check();
                cout << "check\n" << C;
                float f = 0; for (int i=0; i<4; i++) for (int j=0; j<4; j++) f += H[i][j] - C[i][j];
                cout << " -> " << f << endl << endl;
            }
            T.multLeft(U);
            T.mult(Vt);

            // compute translation

            /*vector<Vec3d> pntsT; // looks nice but is wrong :(
            for (auto p : points1) { T.mult(p,p); pntsT.push_back( p ); }
            auto cT = centroid(pntsT);
            T.setTranslate(c2-cT);*/

            Vec3d P;
            T.mult(points1[0], P);
            T.setTranslate(points2[0]-P);

            if (verbose) {
                double f;
                Vec3d Rt, Rs, Rc, ax;
                Quaterniond Rr, Rso;
                T.getTransform(Rt,Rr,Rs,Rso);
                Rr.getValueAsAxisRad(ax,f);
                cout << " Rt: " << Rt << "\n Rr: " << ax << "  " << f << endl;
                cout << " R:\n" << T << endl;
            }
            return T;
        }

        static void test() {
            KabschAlgorithm a;
            vector<Vec3d> p1, p2;

            Pnt3d t(1,2,3);
            //Pnt3d t(0,0,0);
            Quaterniond r(Vec3d(1,0,1), -0.35);

            Matrix4d M;
            M.setTranslate(t);
            M.setRotate(r);

            //p1 = vector<Vec3d>( { Vec3d(1,0,0), Vec3d(1,2,0), Vec3d(1,0,3), Vec3d(4,0,-2) } );
            //p1 = vector<Vec3d>( { Vec3d(1,0,0), Vec3d(-1,0,0), Vec3d(0,0,1), Vec3d(0,0,-1) } );
            p1 = vector<Vec3d>( { Vec3d(0,0,1), Vec3d(0,0,-1), Vec3d(1,1,1), Vec3d(-1,-1,-1), Vec3d(2,2,2) } );
            for (auto v : p1) {
                Pnt3d p(v);
                M.mult(p,p);
                p2.push_back(Vec3d(p));
            }

            a.setPoints1( p1 );
            a.setPoints2( p2 );
            a.setSimpleMatches();
            auto R = a.compute();

            Vec3d ax; double f;
            r.getValueAsAxisRad(ax,f);
            cout << "\nKabschAlgorithm::test\nM:\n" << M << "\n t: " << t << "\n r: " << ax << "  " << f << endl;
            cout << " R\n" << R << endl;

            Vec3d Rt, Rs, Rc;
            Quaterniond Rr, Rso;
            R.getTransform(Rt,Rr,Rs,Rso);
            Rr.getValueAsAxisRad(ax,f);
            cout << " Rt: " << Rt << "\n Rr: " << ax << "  " << f << endl;

            for (int i=0; i<p1.size(); i++) {
                Pnt3d p;
                R.mult(Pnt3d(p1[i]),p);
                cout << " D " << Vec3d(p-p2[i]).length() << " PP " << p2[i] << " / " << p << endl;
            }
        }
};

void VRSkeleton::updateBones() {
    for (auto& b : bones) {
        Bone& bone = b.second;
        auto bJoints = getBoneJoints(b.first);
        if (bJoints.size() <= 1) continue; // ignore EE for now

        vector<Vec3d> pnts1;
        vector<Vec3d> pnts2;

        map<int, Vec3d> jbPositions;

        for (auto e : armature->getOutEdges(b.first)) {
            Vec3d p = joints[e.ID].constraint->getReferenceA()->pos();
            jbPositions[e.ID] = bone.pose.transform( p );
        }

        for (auto e : armature->getInEdges(b.first)) {
            Vec3d p = joints[e.ID].constraint->getReferenceB()->pos();
            jbPositions[e.ID] = bone.pose.transform( p );
        }

        bool verbose = false; //(bone.name == "uArmLeft");

        for (auto j : bJoints) pnts1.push_back( jbPositions[j] );
        for (auto j : bJoints) {
            pnts2.push_back( jointPos(j) );  // TODO: fix points!
            if (verbose) cout << "new joint position, joint: " << joints[j].name << " P: " << joints[j].pos << endl;
        }


        if (verbose) {
            cout << "update bone " << bone.name << ", pose: " << bone.pose.toString() << endl;
            for (auto p1 : pnts1) cout << " p1: " << p1 << endl;
            for (auto p2 : pnts2) cout << " p2: " << p2 << endl;
        }

        KabschAlgorithm a;
        a.setPoints1(pnts1);
        a.setPoints2(pnts2);
        a.setSimpleMatches();
        auto M = a.compute(verbose && 0);
        M.mult( bone.pose.asMatrix() );
        bone.pose = Pose(M);
    }

    // set EE bone poses
    for (auto ee : endEffectors) {
        auto& bone = bones[ee.second.boneID];
        auto& joint = joints[ee.second.jointID];

        auto pose = *ee.second.target;
        pose.setPos(Vec3d(0,0,0));
        auto o = pose.transform( joint.constraint->getReferenceB()->pos() );

        bone.pose = pose;
        bone.pose.setPos( joint.pos - o );
    }

    // compute up vectors for limbs through interpolation between EE and bones with more than 2 joints
    map<int, Vec2i> jointBonesNjoints; // TODO: optimize by computing all chains only once!
    vector< vector<int> > subChains; // sub chains to interpolate

    for (auto j : joints) {
        auto bJoints1 = getBoneJoints(j.second.bone1);
        auto bJoints2 = getBoneJoints(j.second.bone2);
        jointBonesNjoints[j.first] = Vec2i(bJoints1.size(), bJoints2.size());
    }

    for (auto c : ChainDataMap) {
        vector<int> subChain;
        for (auto j : c.second.joints) {
            Joint& joint = joints[j];
            int b1 = joint.bone1;
            int b2 = joint.bone2;
            Vec2i Nj = jointBonesNjoints[j];
            if (Nj[0] != 2 && Nj[1] == 2) { subChain.push_back(b1); subChain.push_back(b2); } // first sub joint
            if (Nj[0] == 2 && Nj[1] == 2) subChain.push_back(b2); // mid sub joint
            if (Nj[0] == 2 && Nj[1] != 2) { subChain.push_back(b2); subChains.push_back(subChain); subChain.clear(); } // end sub joint
        }
    }

    for (auto& subChain : subChains) {
        int N = subChain.size();
        Vec3d up1 = bones[subChain[0]].pose.up();
        Vec3d up2 = bones[subChain[N-1]].pose.up();
        for (int i=1; i<N-1; i++) {
            float k = float(i)/(N-1);
            Vec3d u = up1 + (up2-up1)*k;
            bones[subChain[i]].pose.setUp(u);
        }
    }
}

vector<VRSkeleton::Joint> VRSkeleton::getChain(string endEffector) {
    vector<Joint> res;
    auto bChain = getBonesChain(endEffector);
    auto jChain = getJointsChain(bChain);
    for (auto j : jChain) res.push_back(joints[j]);
    return res;
}

void VRSkeleton::move(string endEffector, PosePtr pose) {
    //KabschAlgorithm::test();
    //return;

    endEffectors[endEffector].target = pose;
    resolveKinematics();
}

map<string, VRSkeleton::EndEffector> VRSkeleton::getEndEffectors() { return endEffectors; }

/* working on constrained FABRIK

- joint is
    - position
    - constraint
    - prev bone
    - next bone

- arm chain
    joints:            waist    shoulder    elbow    wrist
    bones :     abdomen--|--back----|---uArm--|--lArm--|--hand

    1) doFabrik FB on joints
        - new wrist position
        - move elbow towards wrist to satisfy lArm length
        - project shoulder cone generated from orientation constraint
        - move shoulder towards elbow to satisfy uArm length


- interprete constraint:
    - define dir1 and dir2 for each joint
    - dot between dir1 and dir2 corresponds to rotation around x
    - define up1 and up2 for each joint
    - dot between up1 and up2 corresponds to rotation around z
    - compute those vectors based on
        - vectors between joint positions
        - reference positions of each joint

*/



