#include "VRHumanoid.h"
#include "VRSkeleton.h"
#include "VRSkin.h"
#include "core/utils/toString.h"
#include "core/objects/geometry/VRGeoData.h"

using namespace OSG;

VRHumanoid::VRHumanoid(string name) : VRGeometry(name) {
    colors["skin"] = Color3f(1,0.7,0.4);
    colors["shirt"] = Color3f(1,1,0);
    colors["pants"] = Color3f(0,0,1);

    ringParams[30] = { Vec3d(0.1, 0.05, 0.3), Vec3d(0.20, 0.05, 0.3), Vec3d(0.255, 0.1, 0.5) };
    ringParams[31] = { Vec3d(0.1, 0.05, 0.3), Vec3d(0.15, 0.05, 0.3), Vec3d(0.225, 0.1, 0.5) };
    ringParams[32] = { Vec3d(0.2, 0.05, 0.4), Vec3d(0.22, 0.05, 0.3), Vec3d(0.150, 0.1, 0.5) };

    ringParams[40] = { Vec3d(0.1, 0.05, 0.3), Vec3d(0.20, 0.05, 0.3), Vec3d(0.34, 0.1, 0.5) };
    ringParams[41] = { Vec3d(0.1, 0.05, 0.3), Vec3d(0.15, 0.05, 0.3), Vec3d(0.30, 0.1, 0.5) };
    ringParams[42] = { Vec3d(0.1, 0.05, 0.3), Vec3d(0.22, 0.05, 0.3), Vec3d(0.26, 0.1, 0.5) };

    skeleton = VRSkeleton::create();
    skeleton->setupSimpleHumanoid();
    skin = VRSkin::create(skeleton);
}

VRHumanoid::~VRHumanoid() {}

VRHumanoidPtr VRHumanoid::create(string name) {
    auto p = VRHumanoidPtr( new VRHumanoid(name) );
    p->generate();
    return p;
}

VRHumanoidPtr VRHumanoid::ptr() { return static_pointer_cast<VRHumanoid>(shared_from_this()); }
VRSkinPtr VRHumanoid::getSkin() { return skin; }
VRSkeletonPtr VRHumanoid::getSkeleton() { return skeleton; }

void VRHumanoid::generate() {
    VRGeoData data;
    generateTorso(data);
    generateLegs(data);
    generateArms(data);
    generateHead(data);
    data.apply(ptr());
    updateNormals(true);
    cout << "VRHumanoid::generate " << getName() << " " << data.size() << endl;
}

void VRHumanoid::generateTorso(VRGeoData& data) {
    double O = 0.75; // distance to ground
    double H = 0.77; // torso height

    double B1 = 0.42; // hips
    double B2 = 0.4; // belly
    double B3 = 0.4; // ribs
    double B4 = 0.45; // shoulders

    double D1 = 0.26; // hips
    double D2 = 0.27; // belly
    double D3 = 0.24; // ribs
    double D4 = 0.2; // shoulders

    auto bones = skeleton->getBones();
    Vec3d n(0,-1,0);

    Color3f col = colors["shirt"];

    auto addRect = [&](float h, float b, float d, int bID, int rID, vector<float> t) {
        int i1 = data.pushVert(Vec3d(-b*0.5,h,-d*0.5), n, col, Vec2d(0,rID));
        int i2 = data.pushVert(Vec3d( b*0.5,h,-d*0.5), n, col, Vec2d(0,rID));
        int i3 = data.pushVert(Vec3d( b*0.5,h, d*0.5), n, col, Vec2d(0,rID));
        int i4 = data.pushVert(Vec3d(-b*0.5,h, d*0.5), n, col, Vec2d(0,rID));
        auto& bone = bones[bID];
        for (int i=0; i<4; i++) skin->setMap(bone.ID, t);
        return Vec4i(i1,i2,i3,i4);
    };

    auto joinRects = [&](Vec4i r1, Vec4i r2, bool sidesOpen = false) {
        data.pushQuad(r1[0], r1[1], r2[1], r2[0]);
        if (!sidesOpen) data.pushQuad(r1[1], r1[2], r2[2], r2[1]);
        data.pushQuad(r1[2], r1[3], r2[3], r2[2]);
        if (!sidesOpen) data.pushQuad(r1[3], r1[0], r2[0], r2[3]);
    };

    int bID = 12;
    Vec4i i0 = addRect(O, 0.1, 0.22, bID, 10, {1});
    Vec4i i1 = addRect(O+0.14, B1, D1, bID, 11, {1});
    Vec4i i2 = addRect(O+H*0.34, B2, D2, bID, 12, {1});
    Vec4i i3 = addRect(O+H*0.57, B3, D3, bID, 13, {1});
    Vec4i i4 = addRect(O+H*0.75, B4*0.9, D4, bID, 14, {1});
    Vec4i i5 = addRect(O+H*0.94, B4, D4, bID, 15, {1});
    Vec4i i6 = addRect(O+H, 0.15, D4*0.8, bID, 16, {1});

    joinRects(i0, i1, true);
    joinRects(i1, i2);
    joinRects(i2, i3);
    joinRects(i3, i4);
    joinRects(i4, i5, true);
    joinRects(i5, i6);
    data.pushQuad(i0[0], i0[1], i0[2], i0[3]); // cap bottom
    data.pushQuad(i6[0], i6[1], i6[2], i6[3]); // cap top
}

void VRHumanoid::generateHead(VRGeoData& data) {
    double O = 0.75+0.77; // distance to ground
    double H1 = 0.05; // height neck
    double H2 = 0.25; // height head

    auto bones = skeleton->getBones();
    auto& bone = bones[13];
    Vec3d n(0,1,0);

    Color3f col = colors["skin"];

    auto addRing = [&](float h, int rID, vector<float> rads) {
        vector<int> ids;
        int N = (rads.size()-1)*2;
        float da = 2*Pi/N;
        for (int i=0; i<N; i++) {
            double a = da*i;
            int ri = i < rads.size() ? i : -i+2*(rads.size()-1);
            double r = rads[ri];
            int id = data.pushVert(Vec3d(sin(a)*r,h,cos(a)*r), n, col, Vec2d(0,rID));
            skin->addMap(bone.ID, 1);
            ids.push_back(id);
        }
        return ids;
    };

    auto joinRings = [&](vector<int> r1, vector<int> r2) {
        int N = r1.size();
        for (int i=0; i<N; i++) {
            int j = (i+1)%N;
            data.pushQuad(r1[i], r1[j], r2[j], r2[i]);
        }
    };

    auto capRing = [&](vector<int> r) {
        int N = r.size();
        Vec3d pm;
        for (auto i : r) pm += Vec3d( data.getPosition(i) );
        pm *= (1.0/N);
        int c = data.pushVert(pm, n, col, Vec2d(0,0));
        skin->addMap(bone.ID, 1);
        for (int i=0; i<N; i++) {
            int j = (i+1)%N;
            data.pushTri(c, r[i], r[j]);
        }
    };

    auto ids1 = addRing(O, 20, {0.04, 0.06, 0.06, 0.07});
    auto ids2 = addRing(O+H1, 21, {0.04, 0.06, 0.06, 0.07});
    auto ids3 = addRing(O+H1, 22, {0.08, 0.08, 0.08, 0.08});
    auto ids4 = addRing(O+H1+H2*0.3, 23, {0.1, 0.09, 0.09, 0.1});
    auto ids5 = addRing(O+H1+H2*0.5, 24, {0.08, 0.09, 0.09, 0.1});
    auto ids6 = addRing(O+H1+H2*0.75, 25, {0.09, 0.09, 0.09, 0.095});
    auto ids7 = addRing(O+H1+H2*0.9, 26, {0.085, 0.07, 0.07, 0.085});
    auto ids8 = addRing(O+H1+H2, 27, {0.05, 0.04, 0.04, 0.05});

    joinRings(ids1, ids2);
    joinRings(ids2, ids3);
    joinRings(ids3, ids4);
    joinRings(ids4, ids5);
    joinRings(ids5, ids6);
    joinRings(ids6, ids7);
    joinRings(ids7, ids8);

    capRing(ids8);
}

void VRHumanoid::generateLegs(VRGeoData& data) {
    float L1 = 0.38;
    float L2 = 0.35;

    Vec4i hipLIDs(4, 0, 3, 7);
    Vec4i hipRIDs(1, 5, 6, 2);

    auto bones = skeleton->getBones();
    Vec3d n(0,-1,0);

    Color3f col = colors["pants"];

    auto addRect = [&](float h, float D, int bID, int rID, vector<float> t) {
        auto params = ringParams[rID];
        float b = params[0][0];
        float d = params[1][0];
        float x = params[2][0]*D;
        int i1 = data.pushVert(Vec3d(-b*0.5+x,h,-d*0.5), n, col, Vec2d(0,rID));
        int i2 = data.pushVert(Vec3d( b*0.5+x,h,-d*0.5), n, col, Vec2d(0,rID));
        int i3 = data.pushVert(Vec3d( b*0.5+x,h, d*0.5), n, col, Vec2d(0,rID));
        int i4 = data.pushVert(Vec3d(-b*0.5+x,h, d*0.5), n, col, Vec2d(0,rID));
        auto& bone = bones[bID];
        for (int i=0; i<4; i++) skin->setMap(bone.ID, t);
        return Vec4i(i1,i2,i3,i4);
    };

    auto joinRects = [&](Vec4i r1, Vec4i r2) {
        data.pushQuad(r1[0], r1[1], r2[1], r2[0]);
        data.pushQuad(r1[1], r1[2], r2[2], r2[1]);
        data.pushQuad(r1[2], r1[3], r2[3], r2[2]);
        data.pushQuad(r1[3], r1[0], r2[0], r2[3]);
    };

    auto addLeg = [&](float x, int b0) {
        auto i1 = addRect(0, x, b0, 30, {1});
        auto i2 = addRect(L1, x, b0+2, 31, {1});
        auto i3 = addRect(L1+L2, x, b0+1, 32, {1});
        joinRects(i1, i2);
        joinRects(i2, i3);
        joinRects(i3, x<0? hipLIDs : hipRIDs);
        data.pushQuad(i1[0], i1[1], i1[2], i1[3]);
    };

    addLeg(-1, 3);
    addLeg( 1, 0);
}

void VRHumanoid::generateArms(VRGeoData& data) {
    float L0 = 0.7;
    float L1 = 1.1;
    float L2 = 1.3;
    int k = 4*4;

    Vec4i shoulderLIDs(k+4, k+0, k+3, k+7);
    Vec4i shoulderRIDs(k+1, k+5, k+6, k+2);

    auto bones = skeleton->getBones();
    Vec3d n(0,-1,0);

    Color3f col = colors["skin"];

    auto addRect = [&](float h, float D, int bID, int rID, vector<float> t) {
        auto params = ringParams[rID];
        float b = params[0][0];
        float d = params[1][0];
        float x = params[2][0]*D;
        int i1 = data.pushVert(Vec3d(-b*0.5+x,h,-d*0.5), n, col, Vec2d(0,rID));
        int i2 = data.pushVert(Vec3d( b*0.5+x,h,-d*0.5), n, col, Vec2d(0,rID));
        int i3 = data.pushVert(Vec3d( b*0.5+x,h, d*0.5), n, col, Vec2d(0,rID));
        int i4 = data.pushVert(Vec3d(-b*0.5+x,h, d*0.5), n, col, Vec2d(0,rID));
        auto& bone = bones[bID];
        for (int i=0; i<4; i++) skin->setMap(bone.ID, t);
        return Vec4i(i1,i2,i3,i4);
    };

    auto joinRects = [&](Vec4i r1, Vec4i r2) {
        data.pushQuad(r1[0], r1[1], r2[1], r2[0]);
        data.pushQuad(r1[1], r1[2], r2[2], r2[1]);
        data.pushQuad(r1[2], r1[3], r2[3], r2[2]);
        data.pushQuad(r1[3], r1[0], r2[0], r2[3]);
    };

    auto addArm = [&](float x, int b0) {
        auto i1 = addRect(L0, x, b0  , 40, {1});
        auto i2 = addRect(L1, x, b0+2, 41, {1});
        auto i3 = addRect(L2, x, b0+1, 42, {1});
        joinRects(i1, i2);
        joinRects(i2, i3);
        joinRects(i3, x<0? shoulderLIDs : shoulderRIDs);
        data.pushQuad(i1[0], i1[1], i1[2], i1[3]);
    };

    addArm(-1, 9);
    addArm( 1, 6);
}

string VRHumanoid::getParameterString() {
    string data = "{\n";

    data += "\t\"colors\": {\n";
    for (auto col : colors) {
        data += "\t\t\""+col.first+"\": \""+toString(col.second)+"\",\n";
    }
    data += "\t}\n";

    data += "\t\"rings\": {\n";
    for (auto ring : ringParams) {
        data += "\t\t\""+toString(ring.first)+"\": \""+toString(ring.second)+"\",\n";
    }
    data += "\t}\n";

    return data+"}";
}

Color3f VRHumanoid::getColor(string pID) { return colors[pID]; }
vector<Vec3d> VRHumanoid::getRingParams(int rID) { return ringParams[rID]; }

void VRHumanoid::setColor(string pID, Color3f c) {
    colors[pID] = c;
    generate();
}

void VRHumanoid::setRingParams(int rID, vector<double> params) {
    for (int i=0; i<params.size() && i<ringParams[rID].size(); i++) {
        ringParams[rID][i][0] = params[i];
    }
    generate();
}

