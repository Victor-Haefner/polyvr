#include "VRFABRIK.h"
#include "core/utils/toString.h"
#include "core/math/polygon.h"
#include "core/math/triangulator.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"

#include <stack>
#include <algorithm>

using namespace OSG;

template<> string typeName(const FABRIK& k) { return "FABRIK"; }

FABRIK::FABRIK() {}
FABRIK::~FABRIK() {}

FABRIKPtr FABRIK::create() { return FABRIKPtr( new FABRIK() ); }

void FABRIK::addJoint(int ID, PosePtr p) {
    Joint j;
    j.ID = ID;
    j.p = p;
    joints[ID] = j;
}

PosePtr FABRIK::getJointPose(int ID) { return joints[ID].p; }

void FABRIK::addChain(string name, vector<int> joints) {
    Chain c;
    c.name = name;
    c.joints = joints;
    for (int i=0; i<joints.size()-1; i++) {
        auto p1 = this->joints[joints[i  ]].p->pos();
        auto p2 = this->joints[joints[i+1]].p->pos();
        c.distances.push_back((p2-p1).length());
    }

    auto has = [](vector<int> v, int i) {
        return (find(v.begin(), v.end(), i) != v.end());
    };

    for (int i=0; i<joints.size(); i++) {
        auto& in = this->joints[joints[i]].in;
        auto& out = this->joints[joints[i]].out;
        if (i > 0 && has(in, joints[i-1]) == 0) in.push_back(joints[i-1]);
        if (i < joints.size()-1 && has(out, joints[i+1]) == 0) out.push_back(joints[i+1]);
    }
    chains[name] = c;
    updateExecutionQueue();
}

vector<int> FABRIK::getChainJoints(string name) { return chains[name].joints; }

void FABRIK::addConstraint(int j, Vec4d angles) {
    joints[j].constrained = true;
    joints[j].constraintAngles = angles;
}

Vec3d FABRIK::movePointTowards(int j, Vec3d target, float t) {
    auto interp = [](Vec3d& a, Vec3d& b, float t) {
        return a*t + b*(1-t);
    };

    auto& J = joints[j];
    Vec3d pOld = J.p->pos();
    J.p->setPos( interp(pOld, target, t) );
    return pOld;
};

Vec3d FABRIK::moveToDistance(int j1, int j2, float d) {
    auto& J1 = joints[j1];
    auto& J2 = joints[j2];

    Vec3d pOld = J1.p->pos();
    Vec3d D = J2.p->pos() - J1.p->pos();

    if (J2.constrained) {
        Vec3d cU =-J2.p->up(); cU.normalize();
        Vec3d cX = J2.p->x();  cX.normalize();
        Vec3d cD = J2.p->dir();cD.normalize();
        float y = D.dot(cU);
        float x = D.dot(cX);
        float h = D.dot(cD);
        float a = atan2(y,x);
        if (a < 0) a += 2*Pi;

        auto angles = J2.constraintAngles;
        float A = 0, B = 0;
        if (a >= 0      && a <  Pi*0.5) { A = angles[0]; B = angles[1]; }
        if (a >= Pi*0.5 && a <  Pi*1.0) { A = angles[2]; B = angles[1]; }
        if (a >= Pi*1.0 && a <  Pi*1.5) { A = angles[2]; B = angles[3]; }
        if (a >= Pi*1.5 && a <= Pi*2.0) { A = angles[0]; B = angles[3]; }

        float ex = h*tan(A)*cos(a);
        float ey = h*tan(B)*sin(a);
        float er2 = ex*ex+ey*ey;
        float r2  = x*x+y*y;
        if (er2 < r2) {
            J1.p->setPos( J2.p->transform( Vec3d(ex, ey, h) ) );
            D = J2.p->pos() - J1.p->pos();
        }
        /*cout << "AAA, outside? " << bool(er2 < r2) << ", j1 " << j1 << ", j2 " << j2 << " , A " << A << ", a " << a << ", er " << sqrt(er2) << ", r " << sqrt(r2) << endl;

        Vec3d p1(x, y, h);
        Vec3d p2(ex, ey, h);
        J2.debugPnt1 = J2.p->transform(p1);
        J2.debugPnt2 = J2.p->transform(p2);*/
    }

    float L = D.length();
    float li = d / L;
    movePointTowards(j1, J2.p->pos(), li);
    Vec3d nD = J2.p->pos() - J1.p->pos();
    nD.normalize();
    J1.p->setDir(nD);
    J1.p->makeUpOrthogonal();
    return pOld;
}

void FABRIK::forward(Chain& chain) {}
void FABRIK::backward(Chain& chain) {}
void FABRIK::chainIteration(Chain& chain) {}

void FABRIK::setTarget(int i, PosePtr p) {
    joints[i].target = p;
    updateExecutionQueue();
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
                //cout << " add step: " << ee << " " << b << " " << i1 << " " << i2 << " " << chain.name << endl;
                executionQueue.push_back( step(ee, b, i1, i2, chain.name, joints[ee].target, false, true) );
                break;
            }
        }
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
            int ee = c.second.joints.back();
            if (!joints[ee].target) continue;
            auto targetPos = joints[ee].target->pos();
            float distTarget = (joints[ee].p->pos()-targetPos).length();
            cout << "convergence: " << distTarget << ", " << distTarget/tolerance << endl;
            if (distTarget > tolerance) return false;
        }
        return true;
    };

    map<int,vector<Vec3d>> knotPositions;

    for (int i=0; i<10; i++) {
        if (checkConvergence()) break;
        cout << "exec FABRIK iteration " << i << endl;

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
                    auto pOld = moveToDistance(chain.joints[i], chain.joints[i-1], chain.distances[i-1]);
                }
            } else {
                movePointTowards(j.joint, targetPos, 0);
                for (int i = j.i1; i >= j.i2; i--) { // bis Nj-2 bis 1
                    auto pOld = moveToDistance(chain.joints[i], chain.joints[i+1], chain.distances[i]);
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
    }
}

void FABRIK::iterateChain(string chain) {
    if (!chains.count(chain)) return;
    chainIteration(chains[chain]);
}

void FABRIK::visualize(VRGeometryPtr geo) {
    VRGeoData data;

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

    data.apply(geo);
    auto m = VRMaterial::get("fabrikPnts");
    m->setPointSize(5);
    m->setLit(0);
    geo->setMaterial(m);


    // constraints
    VRGeoData cones;
    double R = 0.4;

    for (auto j : joints) {
        if (!j.second.constrained) continue;
        Pnt3d P0 = Pnt3d(j.second.p->pos());
        int v0ID = cones.pushVert(P0, Vec3d(0,0,-1));
        for (int i=0; i<=32; i++) {
            float a = 2*Pi*i/32.0;
            auto angles = j.second.constraintAngles;
            float A = 0, B = 0;
            if (a >= 0      && a <  Pi*0.5) { A = angles[0]; B = angles[1]; }
            if (a >= Pi*0.5 && a <  Pi*1.0) { A = angles[2]; B = angles[1]; }
            if (a >= Pi*1.0 && a <  Pi*1.5) { A = angles[2]; B = angles[3]; }
            if (a >= Pi*1.5 && a <= Pi*2.0) { A = angles[0]; B = angles[3]; }

            float x = R*tan(A)*cos(a);
            float y = R*tan(B)*sin(a);
            Vec3d v = Vec3d(x,y,R);
            Vec3d n = Vec3d(x,y,0);
            v = j.second.p->transform(v, false);
            n = j.second.p->transform(n, false);
            int vID = cones.pushVert(P0 + v, n);
            if (i > 0) cones.pushTri(v0ID, vID, vID-1);
        }
    }

    auto cgeo = cones.asGeometry("kcones");
    geo->addChild(cgeo);
}










