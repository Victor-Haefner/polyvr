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

void FABRIK::setJoint(int ID, PosePtr p) {
    joints[ID].p = p;
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
    Vec3d D = J1.p->pos() - J2.p->pos();

    if (J2.constrained && 0) {
        Vec3d cU =-J2.p->up(); cU.normalize();
        Vec3d cX = J2.p->x();  cX.normalize();
        Vec3d cD = J2.p->dir();cD.normalize();
        float y = D.dot(cU);
        float x = D.dot(cX);
        float h = D.dot(cD);

        if (h < 1e-2) h = 1e-2;

        float a = atan2(y,x);
        if (a < 0) a += 2*Pi;
        cout << " J: " << Vec2i(j1,j2) << ", " << J1.p->pos() << ", " << J2.p->pos() << ", D " << D << endl;
        cout << "  cUXD: " << cU << ", " << cX << ", " << cD << ", D " << D << endl;
        cout << "   XY: " << Vec2d(x,y) << ", a " << a << endl;

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
            Vec3d P = Vec3d(ex, ey, h);
            Vec3d p = J2.p->transform( P );
            J1.p->setPos( p );
            D = J2.p->pos() - J1.p->pos();
            cout << "    Pp: " << P << ", p " << p << endl;
        }
        cout << "      AAA, outside? " << bool(er2 < r2) << ", j1 " << j1 << ", j2 " << j2 << " , A " << A << ", a " << a << ", er " << sqrt(er2) << ", r " << sqrt(r2) << endl;

        /*Vec3d p1(x, y, h);
        Vec3d p2(ex, ey, h);
        J2.debugPnt1 = J2.p->transform(p1);
        J2.debugPnt2 = J2.p->transform(p2);*/
    }

    if (J2.constrained) {
        Vec3d cU = J2.p->up(); cU.normalize();
        Vec3d cX = J2.p->x();  cX.normalize();
        Vec3d cD =-J2.p->dir();cD.normalize();
        float y = D.dot(cU);
        float x = D.dot(cX);
        float h = D.dot(cD);

        float a = atan2(y,x);
        if (a < 0) a += 2*Pi;
        auto angles = J2.constraintAngles;

        float A = 0, B = 0, a1 = 0, a2 = 0;
        if (a >= 0      && a <  Pi*0.5) { A = angles[0]; B = angles[1]; a1 = 0;      a2 = Pi*0.5; }
        if (a >= Pi*0.5 && a <  Pi*1.0) { A = angles[1]; B = angles[2]; a1 = Pi*0.5; a2 = Pi*1.0; }
        if (a >= Pi*1.0 && a <  Pi*1.5) { A = angles[2]; B = angles[3]; a1 = Pi*1.0; a2 = Pi*1.5; }
        if (a >= Pi*1.5 && a <= Pi*2.0) { A = angles[3]; B = angles[0]; a1 = Pi*1.5; a2 = 0; }

        Vec3d v1 = Vec3d(sin(A)*cos(a1), sin(A)*sin(a1), cos(A));
        Vec3d v2 = Vec3d(sin(B)*cos(a2), sin(B)*sin(a2), cos(B));
        v1 = J2.p->transform(v1, false);
        v2 = J2.p->transform(v2, false);

        Vec3d pN = v1.cross(v2); pN.normalize();
        Vec3d p0 = J2.p->pos();

        Vec3d p = J1.p->pos();
        float t = pN.dot(p-p0);
        Vec3d pP = p - pN*t; // projection on plane

        J2.debugPnt1 = p0 + v1*0.1 + v2*0.1;
        J2.debugPnt2 = p;

        if (t < 0) {
            Vec3d pn1 = (pP - p0).cross(v1);
            Vec3d pn2 = (pP - p0).cross(v2);
            float f1 = pn1.dot(pN);
            float f2 = pn2.dot(pN);
            Vec3d kN1 = pN.cross(v1); kN1.normalize();
            Vec3d kN2 = pN.cross(v2); kN2.normalize();

            Vec3d eP1 = pP - kN1 * kN1.dot(pP - p0);
            Vec3d eP2 = pP - kN2 * kN2.dot(pP - p0);
            J2.debugPnt1 = eP1;
            J2.debugPnt2 = eP2;

            if (f1 > 0) pP = eP1;
            if (f2 < 0) pP = eP2;
            /*if (f2 < 1 && f2 < f1) {
                Vec3d kN = pN.cross(v2); kN.normalize();
                //J1.p->setPos( pP - kN * kN.dot(pP - p0) );
            }*/

            J1.p->setPos( pP );

            D = J1.p->pos() - J2.p->pos();
            cout << " xy " << Vec2d(x,y) << ", a " << a << "  t " << t << " p " << p << ", " << f1 << ", " << f2 << "   " << J1.ID << endl;
        }
    }

    float L = D.length();
    float li = d / L;
    movePointTowards(j1, J2.p->pos(), li);

    // update joint direction
    Vec3d nD = -D;
    nD.normalize();
    J1.p->setDir(nD);

    // update joint up vector
    Vec3d u1 = J1.p->up();
    Vec3d u2 = J2.p->up();
    u1 -= nD * u1.dot(nD);
    u2 -= nD * u2.dot(nD);
    Quaterniond q(u1, u2);
    q.multVec(u1, u1);
    u1.normalize();
    J1.p->setUp(u1);
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
            //cout << "convergence: " << distTarget << ", " << distTarget/tolerance << endl;
            if (distTarget > tolerance) return false;
        }
        return true;
    };

    map<int,vector<Vec3d>> knotPositions;

    for (int i=0; i<10; i++) {
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
    double R = 0.1;

    for (auto j : joints) {
        if (!j.second.constrained) continue;
        Pnt3d P0 = Pnt3d(j.second.p->pos());
        int v0ID = cones.pushVert(P0, Vec3d(0,0,-1));
        cout << "constraint of joint " << j.first << endl;
        auto angles = j.second.constraintAngles;

        float a0 = 0, a1 = Pi*0.5, a2 = Pi*1.0, a3 = Pi*1.5;
        Vec3d v0 = Vec3d(sin(angles[0])*cos(a0), sin(angles[0])*sin(a0), cos(angles[0]))*R;
        Vec3d v1 = Vec3d(sin(angles[1])*cos(a1), sin(angles[1])*sin(a1), cos(angles[1]))*R;
        Vec3d v2 = Vec3d(sin(angles[2])*cos(a2), sin(angles[2])*sin(a2), cos(angles[2]))*R;
        Vec3d v3 = Vec3d(sin(angles[3])*cos(a3), sin(angles[3])*sin(a3), cos(angles[3]))*R;

        v0 = j.second.p->transform(v0, false);
        v1 = j.second.p->transform(v1, false);
        v2 = j.second.p->transform(v2, false);
        v3 = j.second.p->transform(v3, false);

        int vID0 = cones.pushVert(P0 + v0, v0);
        int vID1 = cones.pushVert(P0 + v1, v1);
        int vID2 = cones.pushVert(P0 + v2, v2);
        int vID3 = cones.pushVert(P0 + v3, v3);

        cones.pushTri(v0ID, vID0, vID1);
        cones.pushTri(v0ID, vID1, vID2);
        cones.pushTri(v0ID, vID2, vID3);
        cones.pushTri(v0ID, vID3, vID0);

        continue;

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

            /*float t = 0;
            if (a >= 0      && a <  Pi*0.5) { t = a/Pi/0.5;          A = angles[0]; B = angles[1]; }
            if (a >= Pi*0.5 && a <  Pi*1.0) { t = (a-Pi*0.5)/Pi/0.5; A = angles[1]; B = angles[2]; }
            if (a >= Pi*1.0 && a <  Pi*1.5) { t = (a-Pi*1.0)/Pi/0.5; A = angles[2]; B = angles[3]; }
            if (a >= Pi*1.5 && a <= Pi*2.0) { t = (a-Pi*1.5)/Pi/0.5; A = angles[3]; B = angles[0]; }

            Vec3d v1 = Vec3d(sin(A)*cos(a), sin(A)*sin(a), cos(A))*R;
            Vec3d v2 = Vec3d(sin(B)*cos(a), sin(B)*sin(a), cos(B))*R;
            Vec3d d = v2-v1;
            Vec3d k = v1.cross(d); k.normalize();


            if (a >= 0      && a <  Pi*0.5) { A = angles[0]; B = angles[1]; }
            if (a >= Pi*0.5 && a <  Pi*1.0) { A = angles[2]; B = angles[1]; }
            if (a >= Pi*1.0 && a <  Pi*1.5) { A = angles[2]; B = angles[3]; }
            if (a >= Pi*1.5 && a <= Pi*2.0) { A = angles[0]; B = angles[3]; }

            float x = R*tan(A)*cos(a);
            float y = R*tan(B)*sin(a);
            Vec3d v = Vec3d(x,y,R);



            float f = t*Pi*0.5;
            Vec3d v = v1 * (1-t) + v2 * t;
            //float f = v1.enclosedAngle(v2);
            //Vec3d v = v1*cos(t*f) + k.cross(v1)*sin(t*f) + k*k.cross(v1)*(1-cos(t*f));
            Vec3d n = v; n.normalize();*/

            v = j.second.p->transform(v, false);
            n = j.second.p->transform(n, false);
            int vID = cones.pushVert(P0 + v, n);
            if (i > 0) cones.pushTri(v0ID, vID, vID-1);
            //cout << " a " << a << ", tf " << Vec2f(0,f) << ", AB " << Vec2f(A,B) << ", v " << v << endl;
        }
    }

    auto cgeo = cones.asGeometry("kcones");
    geo->addChild(cgeo);
}


/*

TODO: implement constraints with bigger angles

if

up of root is allways given


*/







