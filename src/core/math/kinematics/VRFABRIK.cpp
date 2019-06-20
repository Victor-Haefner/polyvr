#include "VRFABRIK.h"
#include "core/utils/toString.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"

#include <stack>

using namespace OSG;

template<> string typeName(const FABRIK& k) { return "FABRIK"; }

FABRIK::FABRIK() {}
FABRIK::~FABRIK() {}

FABRIKPtr FABRIK::create() { return FABRIKPtr( new FABRIK() ); }

void FABRIK::addJoint(int ID, PosePtr p, vector<int> in, vector<int> out) {
    Joint j;
    j.ID = ID;
    j.p = p;
    j.in = in;
    j.out = out;
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
    chains[name] = c;
}

vector<int> FABRIK::getChainJoints(string name) { return chains[name].joints; }

Vec3d FABRIK::movePointTowards(Chain& chain, int i, Vec3d target, float t) {
    auto interp = [](Vec3d& a, Vec3d& b, float t) {
        return a*t + b*(1-t);
    };

    auto& J = joints[chain.joints[i]];
    Vec3d pOld = J.p->pos();
    J.p->setPos( interp(pOld, target, t) );
    //if (verbose) cout << " movePointTowards: " << J.name << " (" << pOld << ") -> " << 1-t << " / " << target << " -> " << J.pos << endl;
    //if (J.name == "elbowLeft")
    //cout << " movePointTowards: " << J.name << " (" << pOld << ") -> " << 1-t << " / " << target << " -> " << J.pos << endl;
    return pOld;
};

Vec3d FABRIK::moveToDistance(Chain& chain, int i1, int i2, int dID) {
    auto& J1 = joints[chain.joints[i1]];
    auto& J2 = joints[chain.joints[i2]];

    Vec3d pOld = J1.p->pos();
    float li = chain.distances[dID] / (J2.p->pos() - J1.p->pos()).length();
    movePointTowards(chain, i1, J2.p->pos(), li);
    return pOld;
}

void FABRIK::forward(Chain& chain) {}
void FABRIK::backward(Chain& chain) {}
void FABRIK::chainIteration(Chain& chain) {}

void FABRIK::setTarget(int i, PosePtr p) {
    joints[i].target = p;
}

void FABRIK::iterate() {
    struct job {
        int joint;
        int base;
        string chain;
        bool fwd = false;
        PosePtr target;

        job(int j, int b, string c, bool f, PosePtr t) : joint(j), base(b), chain(c), fwd(f), target(t) {};
    };

    vector<job> jobs;

    /*for (auto& c : chains) {
        auto& chain = c.second;
        int J = chain.joints.back();
        job j(J, 0, c.first, false, joints[J].target);
        jobs.push(j);
    }*/

    for (int i=0; i<20; i++) {
        jobs.push_back( job(8,0,"chain2",false,joints[8].target) );
        jobs.push_back( job(8,0,"chain2",true,joints[8].target) );
    }

    for (int i=0; i<10; i++) {
        jobs.push_back( job(5,0,"chain1",false,joints[5].target) );
        jobs.push_back( job(5,0,"chain1",true,joints[5].target) );
    }

    for (auto j : jobs) {
        auto& chain = chains[j.chain];
        if (chain.joints.size() == 0) continue;
        if (!joints.count(j.joint)) continue;
        if (!j.target) continue;


        auto targetPos = j.target->pos();
        float distTarget = (joints[j.joint].p->pos()-targetPos).length();
        if (!j.fwd && distTarget < tolerance) continue;


        cout << "doJob: " << j.chain << ", " << j.joint << " -> " << j.base << ", " << string(j.fwd?"forward":"backward") << endl;

        if (j.fwd) {
            for (int i = 1; i <= chain.distances.size(); i++) { // 1 bis Nj-1
                auto pOld = moveToDistance(chain, i,i-1,i-1);
                if (chain.joints[i] == j.base) break;
            }
        } else {
            int iE = chain.joints.size()-1;
            movePointTowards(chain, iE, targetPos, 0);
            for (int i = chain.distances.size()-1; i > 0; i--) { // bis Nj-2 bis 1
                auto pOld = moveToDistance(chain, i,i+1,i);
                if (chain.joints[i] == j.base) break;
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










