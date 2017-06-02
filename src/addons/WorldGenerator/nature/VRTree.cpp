#include "VRTree.h"

#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/VRLod.h"
#include "core/scene/VRSceneManager.h"
#include "core/math/VRConvexHull.h"
#include "core/utils/VRStorage_template.h"

#include <OpenSG/OSGQuaternion.h>
#include <random>

using namespace OSG;

struct OSG::seg_params : public VRStorage {
    int nodes = 1; //number of iterations
    int child_number = 5; //number of children

    float n_angle = 0.2; //angle between n1 && parent n2
    float p_angle = 0.6; //angle between axis (p1 p2) && parent axis (p1 p2)
    float length = 0.8;
    float radius = 0.1;

    float n_angle_var = 0.2; //n_angle variation
    float p_angle_var = 0.4; //p_angle variation
    float length_var = 0.2; //length variation
    float radius_var = 0.2; //radius variation

    seg_params() {
        store("nodes", &nodes);
        store("child_number", &child_number);
        store("n_angle", &n_angle);
        store("p_angle", &p_angle);
        store("length", &length);
        store("radius", &radius);
        store("n_angle_var", &n_angle_var);
        store("p_angle_var", &p_angle_var);
        store("length_var", &length_var);
        store("radius_var", &radius_var);
    }

    static shared_ptr<seg_params> create() { return shared_ptr<seg_params>( new seg_params() ); }
};

struct OSG::leaf_params : public VRStorage {
    int level = 3; // branch level to add leafs
    int amount = 10; // amount of leafs per segment
    float size = 0.03; // leaf sprite size

    leaf_params() {
        store("level", &level);
        store("amount", &amount);
        store("size", &size);
    }

    static shared_ptr<leaf_params> create() { return shared_ptr<leaf_params>( new leaf_params() ); }
};

struct OSG::segment {
    Vec3f p1 = Vec3f(0,0,0);
    Vec3f p2 = Vec3f(0,1,0);
    Vec3f n1 = Vec3f(0,1,0);
    Vec3f n2 = Vec3f(0,1,0);
    Vec2f radii;
    int lvl = 0;
    segment* parent = 0;
    vector<segment*> children;

    //defaults are for the trunc
    segment(segment* p, float r) {
        parent = p;
        lvl = p ? p->lvl+1 : 0;
        if (p) p1 = p->p2;
        radii = Vec2f(r, r);
        if (p) radii[0] = p->radii[1];
    }
};

VRTree::VRTree(string name) : VRTransform(name) {
    int c = random(0,10);
    if (c == 3) truncColor = Vec3f(0.7, 0.7, 0.7);
    if (c == 4) truncColor = Vec3f(0.7, 0.7, 0.7);
    if (c == 5) truncColor = Vec3f(0.6, 0.5, 0.4);
    if (c == 6) truncColor = Vec3f(0.2, 0.1, 0.1);
    if (c == 7) truncColor = Vec3f(0.3, 0.2, 0);
    if (c == 8) truncColor = Vec3f(0.3, 0.2, 0);
    if (c == 9) truncColor = Vec3f(0.2, 0.1, 0.05);

    store("seed", &seed);
    storeObjVec("branching", parameters, true);
    storeObjVec("foliage", foliage, true);
    regStorageSetupFkt( VRFunction<int>::create("tree setup", boost::bind(&VRTree::setup, this)) );
}

VRTree::~VRTree() {}
VRTreePtr VRTree::create(string name) { return shared_ptr<VRTree>(new VRTree(name)); }
VRTreePtr VRTree::ptr() { return static_pointer_cast<VRTree>( shared_from_this() ); }

void VRTree::setup() {
    grow(seed);
    for (auto lp : foliage) growLeafs(lp);
}

void VRTree::initLOD() {
    lod = VRLod::create("tree_lod");
    lod->setPersistency(0);
    addChild(lod);
    auto lod1 = VRObject::create("tree_lod1");
    auto lod2 = VRObject::create("tree_lod2");
    auto lod3 = VRObject::create("tree_lod3");
    lod1->setPersistency(0);
    lod2->setPersistency(0);
    lod3->setPersistency(0);
    lod->addChild(lod1);
    lod->addChild(lod2);
    lod->addChild(lod3);
    lod->addDistance(20);
    lod->addDistance(50);
}

float VRTree::random (float min, float max) {
    if (max!=min) {
        float c = 1e5;
        return (rand()%(int)(c*max-c*min)+c*min)/c;
    } else return max;
}

float VRTree::variation(float val, float var) { return random(val*(1-var), val*(1+var)); }

Vec3f VRTree::randUVec() { return Vec3f(random(-1,1), random(-1,1), random(-1,1)); }

VRMaterialPtr VRTree::treeMat = 0;
VRMaterialPtr VRTree::leafMat = 0;

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

segment* VRTree::grow(int seed, segment* p, int iteration, float t) {
    this->seed = seed;
    if (parameters.size() <= iteration) return 0;
    auto sp = parameters[iteration];

    if (iteration == 0) { // prepare tree
        branches = vector<segment*>();
        srand(seed);
    }

    segment* s = new segment(p, sp->radius);
    branches.push_back(s);

    if (p) {
        float a = variation(sp->p_angle, sp->p_angle_var);
        float l = variation(sp->length, sp->length_var);
        Vec3f d = p->p2 - p->p1; d.normalize();
        s->p1 = p->p1 + (p->p2-p->p1)*t;
        s->p2 = s->p1 + l*randomRotate(d, a);

        s->n2 = s->p2 - s->p1;
        s->n2.normalize();
        s->n1 = p->n2 + (p->n2 - s->n2)*variation(sp->n_angle, sp->n_angle_var);
    } else {
        s->p2 *= variation(sp->length, sp->length_var);
    }

    for (int n=0; n<sp->nodes; n++) {
        for (int i=0; i<sp->child_number; i++) {
            auto c = grow(seed, s, iteration+1, (n+1)*1.0/sp->nodes);
            if (c) s->children.push_back(c);
        }
    }

    if (iteration == 0) { // finish tree
        trunc = s;
        initMaterials();
        initArmatureGeo();
    }

    return s;
}

void VRTree::initMaterials() {
    if (!treeMat) {
        treeMat = VRMaterial::create("tree_wood");

        treeMat->setDiffuse(Color3f(0.8,0.8,0.6));
        treeMat->setAmbient(Color3f(0.4, 0.4, 0.2));
        treeMat->setSpecular(Color3f(0.1, 0.1, 0.1));

        string wdir = VRSceneManager::get()->getOriginalWorkdir();
        treeMat->readFragmentShader(wdir+"/shader/Trees/Shader_tree_base.fp");
        treeMat->readFragmentShader(wdir+"/shader/Trees/Shader_tree_base.dfp", true);
        treeMat->readVertexShader(wdir+"/shader/Trees/Shader_tree_base.vp");
        treeMat->readGeometryShader(wdir+"/shader/Trees/Shader_tree_base.gp");
        treeMat->readTessControlShader(wdir+"/shader/Trees/Shader_tree_base.tcp");
        treeMat->readTessEvaluationShader(wdir+"/shader/Trees/Shader_tree_base.tep");
        treeMat->setShaderParameter("texture", 0);
        treeMat->enableShaderParameter("OSGCameraPosition");

        VRTextureGenerator tg;
        tg.setSize(Vec3i(50,50,50));
        tg.add("Perlin", 1, truncColor, Vec3f(1,0.9,0.7));
        tg.add("Perlin", 0.25, Vec3f(1,0.9,0.7), truncColor);
        treeMat->setTexture(tg.compose(0));
    }

    if (!leafMat) {
        leafMat = VRMaterial::create("tree_leafs");

        leafMat->setDiffuse(Color3f(0.6,1.0,0.4));
        leafMat->setAmbient(Color3f(0.2, 0.6, 0.2));
        leafMat->setSpecular(Color3f(0.1, 0.1, 0.1));

        string wdir = VRSceneManager::get()->getOriginalWorkdir();
        leafMat->readFragmentShader(wdir+"/shader/Trees/Shader_leafs.fp");
        leafMat->readFragmentShader(wdir+"/shader/Trees/Shader_leafs.dfp", true);
        leafMat->readVertexShader(wdir+"/shader/Trees/Shader_leafs.vp");
        leafMat->readGeometryShader(wdir+"/shader/Trees/Shader_leafs.gp");
        leafMat->setShaderParameter("tex", 0);
        leafMat->setTexture( wdir+"/examples/maple-leaf.png" );
    }
}

void VRTree::initArmatureGeo() {
    if (!lod) initLOD();

    VRGeoData geo[3];
    for (segment* s : branches) {
        geo[0].pushVert(s->p1, s->n1, truncColor, Vec2f(s->radii[0]));
        geo[0].pushVert(s->p2, s->n2, truncColor, Vec2f(s->radii[1]));
        geo[0].pushLine();

        if (s->lvl <= 3) { // first lod
            geo[1].pushVert(s->p1, s->n1, truncColor, Vec2f(s->radii[0]));
            geo[1].pushVert(s->p2, s->n2, truncColor, Vec2f(s->radii[1]));
            geo[1].pushLine();
        }

        if (s->lvl <= 2) { // second lod
            geo[2].pushVert(s->p1, s->n1, truncColor, Vec2f(s->radii[0]));
            geo[2].pushVert(s->p2, s->n2, truncColor, Vec2f(s->radii[1]));
            geo[2].pushLine();
        }
    }

    for (int i=0; i<3; i++) {
        woodGeos.push_back( geo[i].asGeometry("wood2") );
        auto g = woodGeos[i];
        g->setPatchVertices(2);
        g->setType(GL_PATCHES);
        g->setMaterial(treeMat);
        lod->getChild(i)->addChild(g);
    }
}

VRObjectPtr VRTree::copy(vector<VRObjectPtr> children) {
    auto tree = VRTree::create();
    tree->trunc = trunc;
    tree->lod;
    tree->branches = branches;
    tree->truncColor = truncColor;
    tree->woodGeos.push_back( dynamic_pointer_cast<VRGeometry>( children[0]->getChild(0)->getChild(0) ) );
    tree->woodGeos.push_back( dynamic_pointer_cast<VRGeometry>( children[0]->getChild(1)->getChild(0) ) );
    tree->woodGeos.push_back( dynamic_pointer_cast<VRGeometry>( children[0]->getChild(2)->getChild(0) ) );
    tree->leafGeos.push_back( dynamic_pointer_cast<VRGeometry>( children[0]->getChild(0)->getChild(1) ) );
    tree->leafGeos.push_back( dynamic_pointer_cast<VRGeometry>( children[0]->getChild(1)->getChild(1) ) );
    tree->leafGeos.push_back( dynamic_pointer_cast<VRGeometry>( children[0]->getChild(2)->getChild(1) ) );
    tree->leafLodCache = leafLodCache;
    return tree;
}

void VRTree::testSetup() {
    parameters.push_back( seg_params::create() );
    grow(time(0));
}

void VRTree::setup(int branching, int iterations, int seed,
                   float n_angle, float p_angle, float length, float radius,
                   float n_angle_v, float p_angle_v, float length_v, float radius_v) {

    for (int i=0; i<iterations; i++) {
        float r = 0.1*pow(radius,i);
        addBranching(1, branching, n_angle, p_angle, length, r, n_angle_v, p_angle_v, length_v , radius_v);
    }

    grow(seed);
}


void VRTree::addBranching(int nodes, int branching,
           float n_angle, float p_angle, float length, float radius,
           float n_angle_v, float p_angle_v, float length_v , float radius_v) {
    auto sp = seg_params::create();
    sp->nodes = nodes;
    sp->child_number = branching;
    sp->n_angle = n_angle;
    sp->p_angle = p_angle;
    sp->length = length;
    sp->radius = radius;
    sp->n_angle_var = n_angle_v;
    sp->p_angle_var = p_angle_v;
    sp->length_var = length_v;
    sp->radius_var = radius_v;
    parameters.push_back(sp);
}

void VRTree::addLeafs(int lvl, int amount, float size) {
    auto lp = leaf_params::create();
    lp->level = lvl;
    lp->amount = amount;
    lp->size = size;
    foliage.push_back(lp);
    growLeafs(lp);
}

void VRTree::growLeafs(shared_ptr<leaf_params> lp) {
    if (!lod) initLOD();

    if (leafGeos.size() == 0) {
        for (int i=0; i<3; i++) {
            auto g = VRGeometry::create("branches");
            g->setPersistency(0);
            leafGeos.push_back( g );
            lod->getChild(i)->addChild(g);
            g->setMaterial(leafMat);
        }
    }

    random_device rd;
    mt19937 e2(rd());
    normal_distribution<> ndist(0,1);

    auto randVecInSphere = [&](float r) {
        return Vec3f(ndist(e2), ndist(e2), ndist(e2))*r;
    };

    float ca = 1.0; // carotene
    float ch = 1.0; // chlorophyl
    ca = 0.5 + rand()*0.5/RAND_MAX;
    ch = 0.5 + rand()*0.5/RAND_MAX;
    VRGeoData geo0, geo1, geo2;
    for (auto b : branches) {
        if (b->lvl != lp->level) continue;

        // compute branch segment basis
        Vec3f p = (b->p1 + b->p2)*0.5;
        Vec3f d = b->p2 - b->p1;
        Vec3f u = Vec3f(0,1,0);
        Vec3f n = u.cross(d); n.normalize();
        u = d.cross(n); u.normalize();

        float L = d.length();

        for (int i=0; i<lp->amount; i++) {
            Vec3f v = randVecInSphere(L*0.3);
            Vec3f n = p+v-b->p1;
            n.normalize();
            // TODO: add model for carotene and chlorophyl depending on leaf age/size and more..
            geo0.pushVert(p+v, n, Vec3f(lp->size,ca,ch)); // color: leaf size, amount of carotene, amount of chlorophyl
            geo0.pushPoint();
        }
    }
    if (geo0.size() == 0) { cout << "VRTree::addLeafs Warning: no armature with level " << lp->level << endl; return; }

    for (int i=0; i<geo0.size(); i+=4) {
        auto c = geo0.getColor(i); c[0] *= 2; // double lod leaf size
        geo1.pushVert( geo0.getPosition(i), geo0.getNormal(i), c);
        geo1.pushPoint();
    }

    for (int i=0; i<geo0.size(); i+=16) {
        auto c = geo0.getColor(i); c[0] *= 4; // double lod leaf size
        geo2.pushVert( geo0.getPosition(i), geo0.getNormal(i), c);
        geo2.pushPoint();
    }

    geo0.apply(leafGeos[0]);
    geo1.apply(leafGeos[1]);
    geo2.apply(leafGeos[2]);
}

void VRTree::setLeafMaterial(VRMaterialPtr mat) {
    for (auto g : leafGeos) g->setMaterial(mat);
}

void VRTree::createHullLeafLod(VRGeoData& geo, int lvl, Vec3f offset, int ID) {
    Matrix Offset;
    Offset.setTranslate(offset); // TODO, use tree transformation rotation?

    if (leafLodCache.count(lvl)) {
        geo.append(leafLodCache[lvl], Offset);
        return;
    }

    if (leafGeos.size() == 0) return;
    VRGeoData data(leafGeos[0]);

    auto computeHull = [&](VRGeoData& tmpData, Vec4f color) -> VRGeometryPtr {
        if (tmpData.size() == 0) return 0;
        float ca = color[1]; // carotene
        float ch = color[2]; // chlorophyll
        Vec3f leafColor = Vec3f(0.4*ca,0.8*ch,0.2*ch);
        VRConvexHull hull;
        auto hgeo = hull.compute( tmpData.asGeometry("tmpdata") );
        if (!hgeo) return 0;
        auto res = VRGeoData( hgeo );
        for (int i=0; i<res.size(); i++) res.pushColor( leafColor );
        return res.size() > 0 ? res.asGeometry("tmp") : 0;
    };

    int Ns = pow(2,int(4/lvl));
    vector<VRGeoData> clusters(Ns);
    vector<Pnt3f> seeds(Ns);
    for (int k=0; k<Ns; k++) {
        int p = data.size()*k*1.0/Ns;
        seeds[k] = data.getPosition(p);
    }

    auto getMinCluster = [&](const Pnt3f& pos) -> VRGeoData& {
        int cMin = 0;
        float dMin = 1.0e10;
        for (int c = 0; c<Ns; c++) {
            auto& seed = seeds[c];
            float L = (pos-seed).squareLength();
            if (L < dMin) {
                dMin = L;
                cMin = c;
            }
        }
        return clusters[cMin];
    };

    int N = 1000/lvl;
    float D = float(data.size())/N;
    float fuzzy = 0.2;

    random_device rd;
    mt19937 e2(rd());
    normal_distribution<> ndist(0,fuzzy);
    auto randVecInSphere = [&]() {
        return Vec3f(ndist(e2), ndist(e2), ndist(e2));
    };

    Vec4f meanColor;
    for (int i=0; i<N; i++) {
        int j = max( min( int(i*D), data.size()-1), 0);
        meanColor += data.getColor(j);
        Pnt3f pos = data.getPosition(j);
        VRGeoData& cluster = getMinCluster(pos + randVecInSphere());
        cluster.pushVert( pos, data.getNormal(j) );
        cluster.pushPoint();
    }
    meanColor *= 1.0/N;

    VRGeoData Hull;
    for (auto& c : clusters) {
        auto hull = computeHull(c, meanColor);
        if (hull) Hull.append(hull);
    }
    if (Hull.size() == 0) return;

    leafLodCache[lvl] = Hull.asGeometry("lodLeafCache");
    geo.append(Hull, Offset);
}

void VRTree::createHullTrunkLod(VRGeoData& geo, int lvl, Vec3f offset, int ID) {
    Matrix Offset;
    Offset.setTranslate(offset); // TODO, use tree transformation rotation?

    Vec2f id = Vec2f(ID,1); // the 1 is a flag to identify the ID as such!

    if (truncLodCache.count(lvl)) {
        geo.append(truncLodCache[lvl], Offset);
        return;
    }

    if (!trunc) return;

    VRGeoData Hull;

    auto normalize = [](Vec3f v) {
        v.normalize();
        return v;
    };

    auto pushRing = [&](Vec3f p, float r) {
        static Vec3f n1 = normalize( Vec3f(-1,0,-1) );
        static Vec3f n2 = normalize( Vec3f(-1,0, 1) );
        static Vec3f n3 = normalize( Vec3f( 1,0, 1) );
        static Vec3f n4 = normalize( Vec3f( 1,0,-1) );
        int i1 = Hull.pushVert( Pnt3f(-r,0,-r) + p, n1, truncColor, id );
        int i2 = Hull.pushVert( Pnt3f(-r,0, r) + p, n2, truncColor, id );
        int i3 = Hull.pushVert( Pnt3f( r,0, r) + p, n3, truncColor, id );
        int i4 = Hull.pushVert( Pnt3f( r,0,-r) + p, n4, truncColor, id );
        return Vec4i(i1,i2,i3,i4);
    };

    auto pushBox = [&](Vec4i i1, Vec4i i2) {
        Hull.pushQuad(i1[0],i1[1],i2[1],i2[0]);
        Hull.pushQuad(i1[1],i1[2],i2[2],i2[1]);
        Hull.pushQuad(i1[2],i1[3],i2[3],i2[2]);
        Hull.pushQuad(i1[3],i1[0],i2[0],i2[3]);
    };

    int Nlvl = 3;
    if (lvl > 2) Nlvl = 2;
    if (lvl > 3) Nlvl = 1;
    for (segment* s : branches) {
        if (s->lvl > Nlvl) continue;
        auto i0 = pushRing(s->p1, s->radii[0]);
        auto i1 = pushRing(s->p2, s->radii[1]);
        pushBox(i0,i1);
    }

    truncLodCache[lvl] = Hull.asGeometry("truncLodCache");
    geo.append(Hull, Offset);
}






