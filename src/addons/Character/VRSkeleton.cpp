#include "VRSkeleton.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/math/graph.h"
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

void VRSkeleton::setEndEffector(string label, int bone) { if (!endEffectors.count(label)) endEffectors[label] = EndEffector(); endEffectors[label].ID = bone; }
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

	updateGeometry();
}

void VRSkeleton::updateGeometry() {
    VRGeoData geo;
    asGeometry(geo);
    geo.apply( ptr() );

    VRGeoData geo2;
    for (auto j : joints) {
        geo2.pushVert(j.second.pos);
		geo2.pushPoint();
    }
    geo2.apply( jointsGeo );
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
    setEndEffector("head", head);

    // legs
    auto ankle = ballJoint(Vec3d(0,0,-0.25), Vec3d(0,0,0.1));
    auto knee  = hingeJoint(Vec3d(0,0,-0.25), Vec3d(0,0,0.25));
    for (auto i : {-0.25,0.25}) {
        auto hip = ballJoint(Vec3d(i,0,-0.15), Vec3d(0,0,0.25));
        int foot     = addBone(Pose::create(Vec3d(i,0,-0.1),Vec3d(0,0,-1),Vec3d(0,1,0)), 0.2);
        int lowerLeg = addBone(Pose::create(Vec3d(i,0.25,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.5);
        int upperLeg = addBone(Pose::create(Vec3d(i,0.75,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.5);
        addJoint(abdomen, upperLeg, hip);
        addJoint(upperLeg, lowerLeg, knee);
        addJoint(lowerLeg, foot, ankle);
        if (i > 0) setEndEffector("footRight", foot);
        if (i < 0) setEndEffector("footLeft", foot);
    }

    // arms
    auto wrist = ballJoint(Vec3d(0,0,-0.15), Vec3d(0,0,0.05));
    auto elbow = hingeJoint(Vec3d(0,0,-0.15), Vec3d(0,0,0.15));
    for (auto i : {-0.2,0.2}) {
        auto shoulder = ballJoint( Vec3d(-i,0,0.2), Vec3d(0,0,0.15));
        int hand     = addBone(Pose::create(Vec3d(i,1.05,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.1);
        int lowerArm = addBone(Pose::create(Vec3d(i,1.25,0) ,Vec3d(0,-1,0),Vec3d(0,0,1)), 0.3);
        int upperArm = addBone(Pose::create(Vec3d(i,1.55,0) ,Vec3d(0,-1,0),Vec3d(0,0,1)), 0.3);
        addJoint(back, upperArm, shoulder);
        addJoint(upperArm, lowerArm, elbow);
        addJoint(lowerArm, hand, wrist);
        if (i > 0) setEndEffector("handRight", hand);
        if (i < 0) setEndEffector("handLeft", hand);
    }

    setRootBone(abdomen);
}

void VRSkeleton::updateJointPositions() {
    for (auto& j : joints) {
        auto& bone1 = bones[j.second.bone1];
        j.second.pos = bone1.pose.transform( j.second.constraint->getReferenceA()->pos() );
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

void VRSkeleton::simStep(map<string, ChainData>& ChainDataMap) {
    if (simCB) {
        (*simCB)();
        return;
    }

    auto sum = [](vector<float> v) {
        float r = 0;
        for (auto f : v) r += f;
        return r;
    };

    auto doBackAndForth = [&](ChainData& data) {
        vector<int>& joints = data.joints;
        int n = joints.size()-1;

        Vec3d start = jointPos(joints[0]);
        for (int i=n-1; i>= 0; i--) {
            float ri = (jointPos(joints[i+1])-jointPos(joints[i])).length();
            float li = data.d[i]/ri;
            jointPos(joints[i]) = jointPos(joints[i+1])*(1-li) + jointPos(joints[i])*li;
        }
        jointPos(joints[0]) = start;
        for (int i=0; i<n; i++) {
            float ri = (jointPos(joints[i+1])-jointPos(joints[i])).length();
            float li = data.d[i]/ri;
            jointPos(joints[i+1]) = jointPos(joints[i])*(1-li) + jointPos(joints[i+1])*li;
        }
    };

    auto applyFABRIK = [&](ChainData& data) {
        auto targetPos = data.targetPos;
        float tol = 0.001; // 1 mm tolerance
        vector<int>& joints = data.joints;
        vector<float>& distances = data.d;

        // basic FABRIK algorithm
        float Dtarget = (targetPos - jointPos(joints[0])).length();
        if (Dtarget > sum(distances)) { // position unreachable
            for (int i=0; i<distances.size(); i++) {
                float ri = (targetPos-jointPos(joints[i])).length();
                float li = distances[i]/ri;
                jointPos(joints[i+1]) = jointPos(joints[i])*(1-li) + targetPos*li;
            }
        } else { // position reachable
            float difA = (jointPos(joints.back())-targetPos).length();
            int k=0;
            while (difA > tol) {
                k++; if(k>50) break;
                jointPos(joints.back()) = targetPos;
                doBackAndForth(data);
                difA = (jointPos(joints.back())-targetPos).length();
            }
        }
    };

    map<int, SystemData> SystemDataMap;
    map<int, Vec3d> jointPositionsOld = getJointsPositions();

    auto getJointSystems = [&]() {
        for (auto& b : bones) {
            auto joints = getBoneJoints(b.first);
            if (joints.size() <= 2) continue; // not a system!

            auto& data = SystemDataMap[b.first];
            data.joints = joints;
            //data.joints.push_back(joints[0]); // close cycle
            data.bone = b.first;

            cout << "a " << endl;
            for (auto j1 : data.joints) {
                cout << " a " << j1 << endl;
                for (auto j2 : data.joints) {
                    if (j2 == j1) continue;
                    float d = (jointPos(j1) - jointPos(j2)).length();
                    data.d[j1][j2] = d;
                    cout << "  a " << Vec2i(j1,j2) << "   " << d << " " << endl;
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

    getJointSystems();

    applyFABRIK(ChainDataMap["handLeft"]);
    applyFABRIK(ChainDataMap["handRight"]);
    //for (auto e : ChainDataMap) applyFABRIK(e.second);

    for (auto e : SystemDataMap) {
        //doBackAndForth(e.second);
        bool doContinue = true;
        while(doContinue) {
            doContinue = false;
            cout << "b " << endl;
            for (auto j1 : e.second.joints) {
                cout << " b " << j1 << endl;
                for (auto j2 : e.second.joints) {
                    if (j2 == j1) continue;
                    Vec3d M = (jointPos(j1) + jointPos(j2))*0.5;
                    Vec3d D = jointPos(j2) - jointPos(j1);
                    float d = e.second.d[j1][j2];
                    float d2 = D.length();
                    if (abs(d2-d) > 0.001) {
                        D.normalize();
                        D *= d*0.5;
                        jointPos(j1) = M - D;
                        jointPos(j2) = M + D;
                        doContinue = true;
                        cout << "  b " << Vec2i(j1,j2) << "  " << d2 << " " << d << endl;
                    }
                }
            }

            break;

            /*for (int i=1; i<e.second.joints.size(); i++) {
                int jID1 = e.second.joints[i-1];
                int jID2 = e.second.joints[i];
                Vec3d M = (jointPos(jID1) + jointPos(jID2))*0.5;
                Vec3d D = jointPos(jID2) - jointPos(jID1);
                float d = e.second.d[i-1];
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



    auto updateBones = [&](ChainData& data) {
        //cout << "updateBones" << endl;
        for (int i=1; i<data.joints.size(); i++) {
            auto& bone = bones[joints[data.joints[i-1]].bone2];
            int jID1 = data.joints[i-1];
            int jID2 = data.joints[i];

            Vec3d D1 = jointPositionsOld[jID2] - jointPositionsOld[jID1];
            Vec3d D2 = jointPos(jID2) - jointPos(jID1);
            Vec3d pW = jointPositionsOld[jID2] - bone.pose.pos();
            //cout << " joints: " << D1 << "    " << D2 << endl;

            Matrix4d m0, mM;
            m0 = bone.pose.asMatrix();
            mM.setTranslate(jointPos(jID2) - jointPositionsOld[jID2] + Vec3d(m0[3]) + pW);
            mM.setRotate(Quaterniond(D1, D2));
            m0.setTranslate(-pW);
            mM.mult(m0);
            bone.pose = Pose(mM);
        }
    };

    // update bones based on new joint positions
    for (auto e : ChainDataMap) updateBones(e.second);
    //int jID = ChainDataMap[endEffector].joints.back();
    //bones[joints[jID].bone2].pose = *pose;
}

void VRSkeleton::resolveKinematics() {
    auto getBonesChain = [&](string endEffector) {
        int e = endEffectors[endEffector].ID;
        VRPathFinding::Position pR(rootBone);
        VRPathFinding::Position pE(e);
        VRPathFinding pathFinding;
        pathFinding.setGraph(armature);
        auto path = pathFinding.computePath(pR, pE);
        vector<int> chainedBones;
        for (auto p : path) chainedBones.push_back(p.nID);
        return chainedBones;
    };

    auto getJointsChain = [&](vector<int>& chainedBones) {
        vector<int> chainedJoints;
        for (int i=1; i<chainedBones.size(); i++) {
            int nID1 = chainedBones[i-1];
            int nID2 = chainedBones[i];
            int eID = armature->getEdgeID(nID1, nID2);
            chainedJoints.push_back(eID);
        }
        return chainedJoints;
    };

    updateJointPositions();

    map<string, ChainData> ChainDataMap;

    auto getChains = [&]() {
        for (auto e : endEffectors) {
            ChainDataMap[e.first] = ChainData();
            ChainDataMap[e.first].chainedBones = getBonesChain(e.first);
            ChainDataMap[e.first].joints = getJointsChain(ChainDataMap[e.first].chainedBones);
            auto& joint = joints[ ChainDataMap[e.first].joints.back() ];
            auto pose = e.second.target;
            if (pose) ChainDataMap[e.first].targetPos = pose->transform( joint.constraint->getReferenceB()->pos() );
            else ChainDataMap[e.first].targetPos = bones[joint.bone2].pose.transform( joint.constraint->getReferenceB()->pos() );

            for (int i=1; i<ChainDataMap[e.first].joints.size(); i++) {
                int jID1 = ChainDataMap[e.first].joints[i-1];
                int jID2 = ChainDataMap[e.first].joints[i];
                float d = (jointPos(jID1) - jointPos(jID2)).length();
                ChainDataMap[e.first].d.push_back( d );
            }
        }
    };

    getChains();
    simStep(ChainDataMap);
    updateGeometry();
}

void VRSkeleton::move(string endEffector, PosePtr pose) {
    endEffectors[endEffector].target = pose;
    resolveKinematics();
}





