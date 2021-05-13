#include "VRFABRIK.h"
#include "core/utils/toString.h"
#include "core/math/polygon.h"
#include "core/math/triangulator.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"

#include <stack>
#include <algorithm>
#include <OpenSG/OSGQuaternion.h>

#include "core/math/patch.h"

using namespace OSG;


namespace OSG {
    struct FABRIK::Joint {
        int ID;
        string name;
        PosePtr p;
        vector<int> in;
        vector<int> out;
        PosePtr target;
        bool constrained = false;
        Vec4d constraintAngles;
        Vec3d debugPnt1, debugPnt2;
        PatchPtr patch;
        VRObjectPtr patchSurface;

        bool springed = false;
        Vec3d springAnchor;
    };

    struct FABRIK::Chain {
        string name;
        vector<int> joints;
        vector<float> distances;
    };

    struct FABRIK::step {
        int joint;
        int base;
        int i1;
        int i2;
        string chain;
        PosePtr target;
        bool fwd = false;
        bool mid = false;

        step(int j, int b, int i1, int i2, string c, PosePtr t, bool f, bool m) : joint(j), base(b), i1(i1), i2(i2), chain(c), target(t), fwd(f), mid(m) {};
    };
}



FABRIK::FABRIK() {}
FABRIK::~FABRIK() {}

FABRIKPtr FABRIK::create() { return FABRIKPtr( new FABRIK() ); }

void FABRIK::addJoint(int ID, PosePtr p) {
    Joint j;
    j.ID = ID;
    j.p = p;
    joints[ID] = j;
}

void FABRIK::setJoint(int ID, PosePtr p) {
    joints[ID].p = p;
}

PosePtr FABRIK::getJointPose(int ID) { return joints[ID].p; }

void FABRIK::addChain(string name, vector<int> joints) {
    Chain c;
    c.name = name;
    c.joints = joints;
    for (unsigned int i=0; i+1<joints.size(); i++) {
        auto p1 = this->joints[joints[i  ]].p->pos();
        auto p2 = this->joints[joints[i+1]].p->pos();
        c.distances.push_back((p2-p1).length());
    }

    auto has = [](vector<int> v, int i) {
        return (find(v.begin(), v.end(), i) != v.end());
    };

    for (unsigned int i=0; i<joints.size(); i++) {
        auto& in = this->joints[joints[i]].in;
        auto& out = this->joints[joints[i]].out;
        if (i > 0 && has(in, joints[i-1]) == 0) in.push_back(joints[i-1]);
        if (i+1 < joints.size() && has(out, joints[i+1]) == 0) out.push_back(joints[i+1]);
    }
    chains[name] = c;
    updateExecutionQueue();
}

vector<int> FABRIK::getChainJoints(string name) { return chains[name].joints; }

void FABRIK::addSpring(int j, Vec3d anchor) {
    joints[j].springed = true;
    joints[j].springAnchor = anchor;
}


string quatStr(Quaterniond& q) {
    double a; Vec3d d;
    q.getValueAsAxisRad(d,a);
    return "(" + toString(d) + ") " + toString(a);
};

void FABRIK::addConstraint(int j, Vec4d angles) {
    joints[j].constrained = true;
    joints[j].constraintAngles = angles;
    joints[j].patch = Patch::create();

    float xm = (angles[0] - angles[2])*0.5;
    float ym = (angles[1] - angles[3])*0.5;

    float x1 = angles[2] + xm;
    float x2 = angles[0] - xm;
    float y1 = angles[1] - ym;
    float y2 = angles[3] + ym;

    cout << "addConstraint " << angles << " -> M " << Vec2f(xm, ym) << " XY " << Vec4f(x1,x2,y1,y2) << endl;

    float K = -0.3;
    float N = 8;
    float R = 0.1;

    Vec3d p0 (0,0,0);

    Vec3d px1 = Vec3d( -sin(x1),        0, cos(x1) )*R;
    Vec3d px2 = Vec3d(  sin(x2),        0, cos(x2) )*R;
    Vec3d py1 = Vec3d(        0,  sin(y1), cos(y1) )*R;
    Vec3d py2 = Vec3d(        0, -sin(y2), cos(y2) )*R;

    vector<Vec3d> p = {px2, py2, px1, py1};

    Quaterniond Rp = Quaterniond(Vec3d(0,-1,0), xm);
    Rp.mult( Quaterniond(Vec3d(-1,0,0), ym) );
    for (auto& P : p) Rp.multVec(P,P);

    Vec3d t1 = (p[2]-p[0])*0.25;
    Vec3d t2 = (p[1]-p[3])*0.25;

    vector<Vec3d> handles = { p[0]+t2, p[1]-t1, p[3]+t1, p[2]-t2
                            , p[0]-t2, p[3]-t1, p[1]+t1, p[2]+t2
                            , p[0]*K, p[1]*K, p[3]*K, p[2]*K };

    vector<Vec3d> normals(4, Vec3d(0,1,0));
    joints[j].patchSurface = joints[j].patch->fromFullQuad(p, normals, handles, N, true);
}

/** move joint j towards the target position by amount t **/
Vec3d FABRIK::movePointTowards(int j, Vec3d target, float t) {
    auto interp = [](Vec3d& a, Vec3d& b, float t) {
        return a*t + b*(1-t);
    };

    auto& J = joints[j];
    Vec3d pOld = J.p->pos();
    J.p->setPos( interp(pOld, target, t) );
    return pOld;
};

void FABRIK::setDoConstraints(bool b) { doConstraints = b; }
void FABRIK::setDoSprings(bool b) { doSprings = b; }

void FABRIK::applyConstraint(int j) {
    Joint& J1 = joints[j];
    if (J1.in.size() == 0) return;
    Joint& J2 = joints[J1.in[0]]; // TODO: handle multiple in!

    if (J2.constrained && doConstraints) {
        Vec3d pOld = J1.p->pos();
        auto pI = J2.p->transformInv(pOld);
        PosePtr pP = J2.patch->getClosestPose(pI);

        pP = J2.p->multRight(pP);
        Vec3d D = pP->dir();
        float t = (pP->pos() - pOld).dot(D);

        /*Vec3d D = -pP->pos();
        float t = (pP->pos() - pI).dot(D);
        pP = J2.p->multRight(pP);*/

        if (t < 0) J1.p->setPos( pP->pos() );
        J1.debugPnt1 = pP->pos();
        J1.debugPnt2 = pP->pos() + D*0.05;
        //cout << J1.ID << " -> " << J2.ID << " t: " << t << " d: " << D << endl;
    }
}

void FABRIK::applySpring(int j, float d) {
    Joint& J1 = joints[j];
    if (J1.in.size() == 0) return;
    Joint& J2 = joints[J1.in[0]]; // TODO: handle multiple in!

    // test spring force
    if (J2.springed && doSprings) {
        auto pS = Pose::create(J2.springAnchor);
        pS = J2.p->multRight(pS);

        Vec3d D = J1.p->pos() - J2.p->pos();
        //float ts = abs(D.length()/d - 1.0)*0.5;
        float ts = 0.5;

        Vec3d p1 = J1.p->pos();
        Vec3d ps = pS->pos();
        J1.p->setPos( p1 + (ps-p1)*ts);
    }
}

/** move joint j1 to get a distance d to j2, update the up vector of j1, also consider the constraints **/
Vec3d FABRIK::moveToDistance(int j1, int j2, float d, bool constrained, bool fwd) {
    Joint& J1 = joints[j1];
    Joint& J2 = joints[j2];

    Vec3d pOld = J1.p->pos();

    // TODO: extend constraints with a preferred position, implement something like a spring to pull towards that preferred position a bit

    if (constrained) applyConstraint(j1);
    if (!fwd) applySpring(j1, d);

    // move to distance
    Vec3d D = J1.p->pos() - J2.p->pos();
    float L = D.length();
    float li = d / L;
    movePointTowards(j1, J2.p->pos(), li);

    // update joint direction
    Vec3d nD = -D;
    nD.normalize();
    J1.p->setDir(nD);

    // update joint up vector
    //J1.p->makeUpOrthogonal();
    Vec3d u1 = J1.p->up();
    Vec3d u2 = J2.p->up();
    u1 -= nD * u1.dot(nD);
    u2 -= nD * u2.dot(nD);
    Quaterniond q(u1, u2);
    q.multVec(u1, u1);
    u1.normalize();
    //if (J1.ID == 2) cout << " J" << J1.ID << ", u1/u2: " << J1.p->up() << " / " << J2.p->up() << " (" << J1.p->up().length() << "/" << J2.p->up().length() << ")" << endl;
    if (u1.length() > 0.9) J1.p->setUp(u1);
    else J1.p->makeUpOrthogonal();
    return pOld;
}

void FABRIK::forward(Chain& chain) {}
void FABRIK::backward(Chain& chain) {}
void FABRIK::chainIteration(Chain& chain) {}

void FABRIK::setTarget(int i, PosePtr p) {
    if (!joints[i].target) {
        joints[i].target = p;
        updateExecutionQueue();
    } else *joints[i].target = *p;
}

void FABRIK::updateExecutionQueue() {
    executionQueue.clear();

    cout << "FABRIK::updateExecutionQueue" << endl;

    map<int, string> splits;

    for (auto& c : chains) {
        auto& chain = c.second;

        int ee = chain.joints.back();
        int b = 0;
        int i1 = chain.distances.size()-1;
        int i2 = 0;

        for (int i=chain.joints.size()-1; i>=0; i--) {
            auto& J = joints[chain.joints[i]];
            b = chain.joints[i];
            i2 = max(i,1);

            if (J.out.size() > 1) splits[b] = chain.name;

            if (J.out.size() > 1 || i == 0) {
            cout << " add step: " << ee << " " << b << " " << i1 << " " << i2 << " " << chain.name << endl;
            executionQueue.push_back( step(ee, b, i1, i2, chain.name, joints[ee].target, false, true) );
             break;
            }
        }

        /*for (int i=chain.joints.size()-1; i>=0; i--) {
            int jID = chain.joints[i];
            auto& J = joints[jID];
            int i1 = chain.distances.size()-1;
            int i2 = max(i,1);

            if (J.out.size() > 1 || J.target) {
                cout << " add step: " << jID << " " << jID << " " << i1 << " " << i2 << " " << chain.name << endl;
                executionQueue.push_back( step(jID, jID, i1, i2, chain.name, J.target, false, true) );
            }

            if (J.out.size() > 1) {
                splits[jID] = chain.name;
                break;
            }
        }*/
    }

    for (auto s : splits) {
        auto& chain = chains[s.second];

        int ee = chain.joints[s.first];
        int b = 0;
        int i1 = s.first-1;
        int i2 = 0;

        for (int i=s.first-1; i>=0; i--) {
            auto& J = joints[chain.joints[i]];
            b = chain.joints[i];
            i2 = max(i,1);

            if (J.out.size() > 1) splits[b] = chain.name;

            if (J.out.size() > 1 || i == 0) {
                //cout << " add step: " << ee << " " << b << " " << i1 << " " << i2 << " " << chain.name << endl;
                executionQueue.push_back( step(ee, b, i1, i2, chain.name, joints[ee].p, false, false) );
                break;
            }
        }
    }

    // add reverse direction!
    for (int i=executionQueue.size()-1; i>=0; i--) {
        auto s = executionQueue[i];
        s.fwd = true;
        int i1 = s.i2+1;
        if (s.i2 == 1) i1 = 1;
        int i2 = s.i1+1;
        s.i1 = i1;
        s.i2 = i2;
        //cout << " add step: " << s.joint << " " << s.base << " " << s.i1 << " " << s.i2 << " " << s.chain << endl;
        executionQueue.push_back( s );
    }
}

void FABRIK::iterate() {
    auto checkConvergence = [&]() {
        for (auto& c : chains) {
            /*for (auto jID : c.second.joints) {
                auto& J = joints[jID];
                if (!J.target) continue;
                auto targetPos = J.target->pos();
                float distTarget = (J.p->pos()-targetPos).length();
                //cout << "convergence: " << distTarget << ", " << distTarget/tolerance << endl;
                if (distTarget > tolerance) return false;
            }*/

            int ee = c.second.joints.back();
            if (!joints[ee].target) continue;
            auto targetPos = joints[ee].target->pos();
            float distTarget = (joints[ee].p->pos()-targetPos).length();
            //cout << "convergence: " << distTarget << ", " << distTarget/tolerance << endl;
            if (distTarget > tolerance) return false;
        }
        return true;
    };

    map<int,vector<Vec3d>> knotPositions;

    for (int i=0; i<10; i++) {
        //cout << "exec FABRIK iteration " << i << endl;

        for (auto j : executionQueue) {
            //cout << "doJob: " << j.chain << ", " << j.joint << " -> " << j.base << ", " << string(j.fwd?"forward":"backward") << endl;

            auto& chain = chains[j.chain];
            if (chain.joints.size() == 0) continue;
            if (!joints.count(j.joint)) continue;
            if (!j.target) continue;


            /*i1 = 1
            i2 = chain.distances.size()

            i1 = chain.distances.size()-1;
            i2 = 0*/

            auto targetPos = j.target->pos();
            //cout << " " << Vec2i(chain.distances.size()-1,0) << "  " << Vec2i(j.i1, j.i2) << endl;

            if (j.fwd) {
                for (int i = j.i1; i <= j.i2; i++) { // 1 bis Nj-1
                    auto pOld = moveToDistance(chain.joints[i], chain.joints[i-1], chain.distances[i-1], true, true);
                }
            } else {
                movePointTowards(j.joint, targetPos, 0);
                for (int i = j.i1; i >= j.i2; i--) { // bis Nj-2 bis 1
                    auto pOld = moveToDistance(chain.joints[i], chain.joints[i+1], chain.distances[i], true, false);
                }
            }

            if (j.mid) {
                //cout << "  add for centroid: " << j.base << ", " << joints[j.base].out.size() << endl;
                knotPositions[j.base].push_back( joints[j.base].p->pos() );
                if (knotPositions[j.base].size() == joints[j.base].out.size()) { // apply centroid
                    Vec3d c;
                    for (auto p : knotPositions[j.base]) c += p;
                    c *= 1.0/knotPositions[j.base].size();
                    joints[j.base].p->setPos(c);
                    //cout << "   apply centroid: " << j.base << ", " << c << endl;
                    knotPositions[j.base].clear();
                }
            }
        }

        if (checkConvergence()) break;
    }
}

void FABRIK::iterateChain(string chain) {
    if (!chains.count(chain)) return;
    chainIteration(chains[chain]);
}

void FABRIK::visualize(VRGeometryPtr geo) {
    VRGeoData data;
    geo->clearChildren();

    // joint points
    for (auto j : joints) {
        data.pushVert(j.second.p->pos(), Vec3d(0,0,0), Color3f(1,0,0));
        data.pushPoint();
    }

    // chain lines
    for (auto c : chains) {
        int nj = c.second.joints.size();
        for (int i=0; i<nj-1; i++) {
            data.pushLine(c.second.joints[i], c.second.joints[i+1]);
        }
    }

    // joint orientation
    for (auto j : joints) {
        data.pushVert(j.second.p->pos(), Vec3d(0,0,0), Color3f(1,1,0));
        data.pushVert(j.second.p->pos()+j.second.p->up()*0.05, Vec3d(0,0,0), Color3f(1,1,0));
        data.pushLine();
        data.pushVert(j.second.p->pos(), Vec3d(0,0,0), Color3f(0,1,0));
        data.pushVert(j.second.p->pos()+j.second.p->dir()*0.05, Vec3d(0,0,0), Color3f(0,1,0));
        data.pushLine();
        //if (j.second.ID == 2) cout << "vis: " << j.second.p->dir() << ",   " << j.second.p->up() << endl;
    }

    // targets
    for (auto j : joints) {
        if (j.second.target) {
            data.pushVert(j.second.target->pos(), Vec3d(0,0,0), Color3f(0,0,1));
            data.pushPoint();
        }
    }

    // targets
    for (auto j : joints) {
        data.pushVert(j.second.debugPnt1, Vec3d(0,0,0), Color3f(0,1,0));
        data.pushPoint();
        data.pushVert(j.second.debugPnt2, Vec3d(0,0,0), Color3f(0,1,1));
        data.pushPoint();
    }

    // spring anchors
    for (auto j : joints) {
        if (j.second.springed) {
            auto p = j.second.p->transform(j.second.springAnchor);
            data.pushVert(j.second.p->pos(), Vec3d(0,0,0), Color3f(0,0,1));
            data.pushVert(p, Vec3d(0,0,0), Color3f(0,0,1));
            data.pushLine();
        }
    }

    data.apply(geo);
    auto m = VRMaterial::get("fabrikPnts");
    m->setPointSize(5);
    m->setLit(0);
    geo->setMaterial(m);


    // constraints
    VRGeoData cones;
    double R = 0.1;

    for (auto j : joints) {
        if (!j.second.constrained) continue;
        geo->addChild(j.second.patchSurface);
        auto surf = dynamic_pointer_cast<VRGeometry>( j.second.patchSurface->getChild(0) );
        surf->setPose( j.second.p );
    }
}


/*

TODO: implement constraints with bigger angles

if

up of root is allways given


*/







