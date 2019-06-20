#include "VRFABRIK.h"
#include "core/utils/toString.h"
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

/*Vec3d FABRIK::movePointTowards(Chain& chain, int i, Vec3d target, float t) {
    auto interp = [](Vec3d& a, Vec3d& b, float t) {
        return a*t + b*(1-t);
    };

    auto& J = joints[chain.joints[i]];
    Vec3d pOld = J.p->pos();
    J.p->setPos( interp(pOld, target, t) );
    return pOld;
};

Vec3d FABRIK::moveToDistance(Chain& chain, int i1, int i2, int dID) {
    auto& J1 = joints[chain.joints[i1]];
    auto& J2 = joints[chain.joints[i2]];

    Vec3d pOld = J1.p->pos();
    float li = chain.distances[dID] / (J2.p->pos() - J1.p->pos()).length();
    movePointTowards(chain, i1, J2.p->pos(), li);
    return pOld;
}*/

Vec3d FABRIK::movePointTowards(int j, Vec3d target, float t) {
    auto interp = [](Vec3d& a, Vec3d& b, float t) {
        return a*t + b*(1-t);
    };

    auto& J = joints[j];
    Vec3d pOld = J.p->pos();
    J.p->setPos( interp(pOld, target, t) );
    //cout << "  movePoint " << j << " to " << target << endl;
    return pOld;
};

Vec3d FABRIK::moveToDistance(int j1, int j2, float d) {
    auto& J1 = joints[j1];
    auto& J2 = joints[j2];

    Vec3d pOld = J1.p->pos();
    float li = d / (J2.p->pos() - J1.p->pos()).length();
    movePointTowards(j1, J2.p->pos(), li);
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


    /*for (auto& c : chains) {
        auto& chain = c.second;
        int J = chain.joints.back();
        job j(J, 0, c.first, false, joints[J].target);
        jobs.push(j);
    }*/

    /*for (int i=0; i<20; i++) {
        jobs.push_back( job(8,0,"chain2",false,joints[8].target) );
        jobs.push_back( job(8,0,"chain2",true,joints[8].target) );
    }

    for (int i=0; i<10; i++) {
        jobs.push_back( job(5,0,"chain1",false,joints[5].target) );
        jobs.push_back( job(5,0,"chain1",true,joints[5].target) );
    }*/

    executionQueue.push_back( step(5,2,4,1,"chain1",joints[5].target,false,true) );
    executionQueue.push_back( step(8,2,4,1,"chain2",joints[8].target,false,true) );
    executionQueue.push_back( step(2,0,1,0,"chain2",joints[2].p     ,false,false) );
    executionQueue.push_back( step(2,0,1,2,"chain2",joints[2].p     ,true,false) );
    executionQueue.push_back( step(8,2,3,5,"chain2",joints[8].target,true,true) );
    executionQueue.push_back( step(5,2,3,5,"chain1",joints[5].target,true,true) );
}

void FABRIK::iterate() {
    auto checkConvergence = [&]() {
        for (auto& c : chains) {
            int ee = c.second.joints.back();
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
                for (int i = j.i1; i > j.i2; i--) { // bis Nj-2 bis 1
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

    for (auto j : joints) {
        data.pushVert(j.second.p->pos(), Vec3d(0,0,0), Color3f(1,0,0));
        data.pushPoint();
    }

    for (auto j : joints) {
        if (j.second.target) {
            data.pushVert(j.second.target->pos(), Vec3d(0,0,0), Color3f(0,0,1));
            data.pushPoint();
        }
    }

    for (auto c : chains) {
        int nj = c.second.joints.size();
        for (int i=0; i<nj-1; i++) {
            data.pushLine(c.second.joints[i], c.second.joints[i+1]);
        }
    }

    data.apply(geo);

    auto m = VRMaterial::get("fabrikPnts");
    m->setPointSize(5);
    m->setLit(0);
    geo->setMaterial(m);
}










