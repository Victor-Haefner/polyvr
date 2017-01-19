#include "VRTree.h"

#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/VRLod.h"
#include "core/scene/VRSceneManager.h"

#include <OpenSG/OSGQuaternion.h>
#include <random>

using namespace OSG;

struct OSG::seg_params {
    int iterations; //number of iterations
    int child_number; //number of children

    float n_angle; //angle between n1 && parent n2
    float p_angle; //angle between axis (p1 p2) && parent axis (p1 p2)
    float l_factor; //length diminution factor
    float r_factor; //radius diminution factor

    float n_angle_var; //n_angle variation
    float p_angle_var; //p_angle variation
    float l_factor_var; //l_factor variation
    float r_factor_var; //r_factor variation

    seg_params () {
        iterations = 5;
        child_number = 5;

        n_angle = 0.2;
        p_angle = 0.6;
        l_factor = 0.8;
        r_factor = 0.5;

        n_angle_var = 0.2;
        p_angle_var = 0.4;
        l_factor_var = 0.2;
        r_factor_var = 0.2;
    }
};

struct OSG::segment {
    Vec3f p1, p2, n1, n2;
    Vec2f params[2];
    int lvl = 0;
    segment* parent;
    vector<segment*> children;

    //defaults are for the trunc
    segment(int _lvl = 0, segment* _parent = 0,
            Vec3f _p1 = Vec3f(0,0,0),
            Vec3f _n1 = Vec3f(0,1,0),
            Vec3f _p2 = Vec3f(0,1,0),
            Vec3f _n2 = Vec3f(0,1,0)) {
        p1 = _p1;
        p2 = _p2;
        n1 = _n1;
        n2 = _n2;
        lvl = _lvl;

        params[0] = Vec2f(1, 0);
        params[1] = Vec2f(1, 0);

        parent = _parent;
    }
};


VRTree::VRTree() : VRGeometry("tree") {}

VRTreePtr VRTree::create() { return shared_ptr<VRTree>(new VRTree() ); }
VRTreePtr VRTree::ptr() { return static_pointer_cast<VRTree>( shared_from_this() ); }

float VRTree::random (float min, float max) {
    if (max!=min) {
        float c = 1e5;
        return (rand()%(int)(c*max-c*min)+c*min)/c;
    } else return max;
}

float VRTree::variation(float val, float var) { return random(val*(1-var), val*(1+var)); }

Vec3f VRTree::randUVec() { return Vec3f(random(-1,1), random(-1,1), random(-1,1)); }

VRMaterialPtr VRTree::treeMat = 0;

//rotate a vector with angle 'a' in a random direction
Vec3f VRTree::randomRotate(Vec3f v, float a) {
    if (a == 0) return v;

    Vec3f x, d;

    do x = randUVec();
    while (x.dot(v) > 1e-3);

    d = v.cross(x);
    d.normalize();

    Quaternion q(d, a);
    q.multVec(v, v);
    return v;
}

void VRTree::grow(const seg_params& sp, segment* p, int iteration) {
    if (p == 0) return;
    if (iteration == sp.iterations) return;

    for (int i=0;i<sp.child_number;i++) {
        segment* c = new segment(p->lvl+1, p, p->p2);
        branches.push_back(c);
        c->p2 = randomRotate(p->p2 - p->p1, variation(sp.p_angle, sp.p_angle_var));
        c->p2 *= variation(sp.l_factor, sp.l_factor_var);
        c->p2 += p->p2;

        c->n2 = c->p2 - c->p1;
        c->n2.normalize();
        c->n1 = p->n2 + (p->n2 - c->n2)*variation(sp.n_angle, sp.n_angle_var);

        c->params[0] = Vec2f(pow(sp.r_factor,iteration), 0);
        c->params[1] = Vec2f(pow(sp.r_factor,iteration+1), 0);

        p->children.push_back(c);
    }

    for (int i=0;i<sp.child_number;i++) {
        grow(sp, p->children[i], iteration+1);
    }
}

VRMaterialPtr VRTree::initMaterial() {
    VRMaterialPtr mat = VRMaterial::create("tree_mat");

    mat->setDiffuse(Color3f(0.8,0.8,0.6));
    mat->setAmbient(Color3f(0.4, 0.4, 0.2));
    mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    string wdir = VRSceneManager::get()->getOriginalWorkdir();
    mat->readFragmentShader(wdir+"/shader/Trees/Shader_tree_base.fp");
    mat->readFragmentShader(wdir+"/shader/Trees/Shader_tree_base.dfp", true);
    mat->readVertexShader(wdir+"/shader/Trees/Shader_tree_base.vp");
    mat->readGeometryShader(wdir+"/shader/Trees/Shader_tree_base.gp");
    mat->readTessControlShader(wdir+"/shader/Trees/Shader_tree_base.tcp");
    mat->readTessEvaluationShader(wdir+"/shader/Trees/Shader_tree_base.tep");
    mat->setShaderParameter("texture", 0);
    mat->enableShaderParameter("OSGCameraPosition");

	VRTextureGenerator tg;
	tg.setSize(Vec3i(50,50,50));
	tg.add("Perlin", 1, Vec3f(0.7,0.5,0.3), Vec3f(1,0.9,0.7));
	tg.add("Perlin", 0.25, Vec3f(1,0.9,0.7), Vec3f(0.7,0.5,0.3));
	mat->setTexture(tg.compose(0));

    return mat;
}

void VRTree::initArmatureGeo() {
    vector<Vec3f> pos, norms;
    vector<Vec2f> texs;
    vector<int> inds;

    for (uint i=0;i<branches.size();i++) {
        segment* s = branches[i];
        pos.push_back(s->p1);
        pos.push_back(s->p2);
        norms.push_back(s->n1);
        norms.push_back(s->n2);
        inds.push_back(2*i);
        inds.push_back(2*i+1);
        texs.push_back(s->params[0]);
        texs.push_back(s->params[1]);
    }

    VRGeometry::create(GL_PATCHES, pos, norms, inds, texs);
    setPatchVertices(2);
}

void VRTree::testSetup() {
    srand(time(0));
    trunc = new segment();
    branches = vector<segment*>();
    branches.push_back(trunc);
    seg_params sp;
    grow(sp, trunc);
    initArmatureGeo();
    initMaterial();
}

void VRTree::setup(int branching, int iterations, int seed,
                   float n_angle, float p_angle, float l_factor, float r_factor,
                   float n_angle_v, float p_angle_v, float l_factor_v, float r_factor_v) {
    srand(seed);
    trunc = new segment();
    branches = vector<segment*>();
    branches.push_back(trunc);
    seg_params sp;

    sp.child_number = branching;
    sp.iterations = iterations;
    sp.n_angle = n_angle;
    sp.p_angle = p_angle;
    sp.l_factor = l_factor;
    sp.r_factor = r_factor;
    sp.n_angle_var = n_angle_v;
    sp.p_angle_var = p_angle_v;
    sp.l_factor_var = l_factor_v;
    sp.r_factor_var = r_factor_v;

    grow(sp, trunc);
    initArmatureGeo();
    if (!treeMat) treeMat = initMaterial();
    setMaterial(treeMat);
}

void VRTree::addLeafs(int lvl, int amount) { // TODO: add default material!
    if (leafGeos.size() == 0) {
        auto g0 = VRGeometry::create("branches");
        auto g1 = VRGeometry::create("branches");
        auto g2 = VRGeometry::create("branches");
        leafGeos.push_back( g0 );
        leafGeos.push_back( g1 );
        leafGeos.push_back( g2 );

        auto lod = VRLod::create("branches_lod");
        addChild(lod);
        lod->addChild(g0);
        lod->addChild(g1);
        lod->addChild(g2);
        lod->addDistance(20);
        lod->addDistance(50);
    }

    random_device rd;
    mt19937 e2(rd());
    normal_distribution<> ndist(0,1);

    auto randVecInSphere = [&](float r) {
        return Vec3f(ndist(e2), ndist(e2), ndist(e2))*r;
    };

    float s = 0.03;
    VRGeoData geo0, geo1, geo2;
    for (auto b : branches) {
        if (b->lvl != lvl) continue;

        // compute branch segment basis
        Vec3f p = (b->p1 + b->p2)*0.5;
        Vec3f d = b->p2 - b->p1;
        Vec3f u = Vec3f(0,1,0);
        Vec3f n = u.cross(d); n.normalize();
        u = d.cross(n); u.normalize();

        float L = d.length();

        for (int i=0; i<amount; i++) {
            Vec3f v = randVecInSphere(L*0.3);
            Vec3f n = p+v-b->p1;
            n.normalize();
            geo0.pushVert(p+v, n, Vec3f(s,0,0));
            geo0.pushPoint();
        }
    }

    for (int i=0; i<geo0.size(); i+=4) {
        geo1.pushVert( geo0.getPosition(i), geo0.getNormal(i), Vec3f(s*2,0,0));
        geo1.pushPoint();
    }

    for (int i=0; i<geo0.size(); i+=16) {
        geo2.pushVert( geo0.getPosition(i), geo0.getNormal(i), Vec3f(s*4,0,0));
        geo2.pushPoint();
    }

    geo0.apply(leafGeos[0]);
    geo1.apply(leafGeos[1]);
    geo2.apply(leafGeos[2]);
}

void VRTree::setLeafMaterial(VRMaterialPtr mat) {
    for (auto g : leafGeos) g->setMaterial(mat);
}


