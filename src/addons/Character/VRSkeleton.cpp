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
        geo2.pushVert(j.second.pos, Vec3d(0,1,0), j.second.col);
		geo2.pushPoint();
    }

    for (auto e : endEffectors) {
        if (!e.second.target) continue;
        auto& joint = joints[ e.second.jointID ];
        Vec3d p = e.second.target->transform( joint.constraint->getReferenceB()->pos() );
        geo2.pushVert(p, Vec3d(0,1,0), Color3f(1,0.5,0.2));
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
    int abdomen = addBone(Pose::create(Vec3d(0,1.15,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.3, "abdomen");
    int back    = addBone(Pose::create(Vec3d(0,1.5,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.4, "back");
    int head    = addBone(Pose::create(Vec3d(0,1.8,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.2, "head");
    addJoint(abdomen, back, waist, "waist");
    addJoint(back, head, neck, "neck");
    setEndEffector("head", head);

    // legs
    auto ankle = ballJoint(Vec3d(0,0,-0.25), Vec3d(0,0,0.1));
    auto knee  = hingeJoint(Vec3d(0,0,-0.25), Vec3d(0,0,0.25));
    for (auto i : {-0.25,0.25}) {
        string side = i < 0 ? "Left" : "Right";
        Color3f sc = i < 0 ? Color3f(1,0,0) : Color3f(0,0,1);
        auto hip = ballJoint(Vec3d(-i,0,-0.15), Vec3d(0,0,0.25));
        int foot     = addBone(Pose::create(Vec3d(i,0,-0.1),Vec3d(0,0,-1),Vec3d(0,1,0)), 0.2, "foot"+side);
        int lowerLeg = addBone(Pose::create(Vec3d(i,0.25,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.5, "lLeg"+side);
        int upperLeg = addBone(Pose::create(Vec3d(i,0.75,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.5, "uLeg"+side);
        addJoint(abdomen, upperLeg, hip, "hip"+side, sc);
        addJoint(upperLeg, lowerLeg, knee, "knee"+side, sc);
        addJoint(lowerLeg, foot, ankle, "ankle"+side, sc);
        setEndEffector("foot"+side, foot);
    }

    // arms
    auto wrist = ballJoint(Vec3d(0,0,-0.15), Vec3d(0,0,0.05));
    auto elbow = hingeJoint(Vec3d(0,0,-0.15), Vec3d(0,0,0.15));
    for (auto i : {-0.2,0.2}) {
        string side = i < 0 ? "Left" : "Right";
        Color3f sc = i < 0 ? Color3f(1,0,0) : Color3f(0,0,1);
        auto shoulder = ballJoint( Vec3d(-i,0,0.2), Vec3d(0,0,0.15));
        int hand     = addBone(Pose::create(Vec3d(i,1.05,0),Vec3d(0,-1,0),Vec3d(0,0,1)), 0.1, "hand"+side);
        int lowerArm = addBone(Pose::create(Vec3d(i,1.25,0) ,Vec3d(0,-1,0),Vec3d(0,0,1)), 0.3, "lArm"+side);
        int upperArm = addBone(Pose::create(Vec3d(i,1.55,0) ,Vec3d(0,-1,0),Vec3d(0,0,1)), 0.3, "uArm"+side);
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
        j.second.pos = bone1.pose.transform( j.second.constraint->getReferenceA()->pos() );
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

    auto getJointSystems = [&]() {
        for (auto& b : bones) {
            auto joints = getBoneJoints(b.first);
            if (joints.size() <= 2) continue; // not a system!

            auto& data = SystemDataMap[b.first];
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

    getJointSystems();

    applyFABRIK(ChainDataMap["handLeft"]);
    applyFABRIK(ChainDataMap["handRight"]);
    //for (auto e : ChainDataMap) applyFABRIK(e.second);

    for (auto e : SystemDataMap) {
        //doBackAndForth(e.second);
        bool doContinue = true;
        while(doContinue) {
            doContinue = false;
            //cout << "b " << endl;
            for (auto j1 : e.second.joints) {
                //cout << " b " << j1 << endl;
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
                        //cout << "  b " << Vec2i(j1,j2) << "  " << d2 << " " << d << endl;
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

    map<string, ChainData> ChainDataMap;

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

    map<int, Vec3d> jointPositionsOld = getJointsPositions();
    getChains();
    simStep(ChainDataMap);
    updateBones(ChainDataMap, jointPositionsOld);
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
            //T.setScale(Vec3d(1,1,d));
            //T.setScale(Vec3d(1,d,1));
            T.setScale(Vec3d(d,d,d));
            //T.setScale(Vec3d(1,1,1));
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
            vector<Vec3d> pntsT;
            for (auto p : points1) { T.mult(p,p); pntsT.push_back( p ); }
            auto cT = centroid(pntsT);
            T.setTranslate(c2-cT);

            /*Vec3d P;
            T.mult(points1[0], P);
            T.setTranslate(points2[0]-P);*/

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

void VRSkeleton::updateBones(map<string, ChainData>& ChainDataMap, map<int, Vec3d>& jointPositionsOld) {
    auto updateChain = [&](ChainData& data) {
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
    //for (auto e : ChainDataMap) updateChain(e.second);

    for (auto& b : bones) {
        Bone& bone = b.second;
        auto bJoints = getBoneJoints(b.first);
        if (bJoints.size() <= 1) continue;

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

        bool verbose = (bone.name == "uArmRight");

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
        auto M = a.compute(verbose);
        M.mult( bone.pose.asMatrix() );
        bone.pose = Pose(M);
    }

    for (auto ee : endEffectors) {
        auto& bone = bones[ee.second.boneID];
        auto& joint = joints[ee.second.jointID];

        auto pose = *ee.second.target;
        pose.setPos(Vec3d(0,0,0));
        auto o = pose.transform( joint.constraint->getReferenceB()->pos() );

        bone.pose = pose;
        bone.pose.setPos( joint.pos - o );
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





