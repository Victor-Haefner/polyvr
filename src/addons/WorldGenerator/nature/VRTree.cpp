#include "VRTree.h"

#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/material/VRTextureMosaic.h"
#include "core/objects/VRLod.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/math/boundingbox.h"
#ifndef WITHOUT_LAPACKE_BLAS
#include "core/math/VRConvexHull.h"
#endif
#include "core/utils/VRStorage_template.h"
#include "core/utils/system/VRSystem.h"

#include "core/tools/VRTextureRenderer.h"
#include "core/math/pose.h"
#include "core/objects/geometry/sprite/VRSprite.h"

#include <OpenSG/OSGQuaternion.h>
#include <functional>
#include <random>

#include <boost/functional/hash.hpp>

#define GLSL(shader) #shader

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
    Vec3d p1 = Vec3d(0,0,0);
    Vec3d p2 = Vec3d(0,1,0);
    Vec3d n1 = Vec3d(0,1,0);
    Vec3d n2 = Vec3d(0,1,0);
    Vec2d radii;
    int lvl = 0;
    segment* parent = 0;
    vector<segment*> children;

    //defaults are for the trunc
    segment(segment* p, float r) {
        parent = p;
        lvl = p ? p->lvl+1 : 0;
        if (p) p1 = p->p2;
        radii = Vec2d(r, r);
        if (p) radii[0] = p->radii[1];
    }
};

VRTree::VRTree(string name) : VRTransform(name) {
    int c = random(0,10);
    if (c == 0) truncColor = Color4f(0.3, 0.2, 0, 1);
    if (c == 1) truncColor = Color4f(0.6, 0.5, 0.4, 1);
    if (c == 2) truncColor = Color4f(0.2, 0.1, 0.05, 1);
    if (c == 3) truncColor = Color4f(0.2, 0.1, 0.05, 1);
    if (c == 4) truncColor = Color4f(0.3, 0.2, 0, 1);
    if (c == 5) truncColor = Color4f(0.6, 0.5, 0.4, 1);
    if (c == 6) truncColor = Color4f(0.2, 0.1, 0.1, 1);
    if (c == 7) truncColor = Color4f(0.3, 0.2, 0, 1);
    if (c == 8) truncColor = Color4f(0.3, 0.2, 0, 1);
    if (c == 9) truncColor = Color4f(0.2, 0.1, 0.05, 1);
    if (c ==10) truncColor = Color4f(0.2, 0.1, 0.05, 1);

    // desaturate and make brighter
    float a = 0.3;
    truncColor = truncColor*(1-a) + Color4f(a,a,a,1);

    store("seed", &seed);
    storeObjVec("branching", parameters, true);
    storeObjVec("foliage", foliage, true);
    regStorageSetupFkt( VRStorageCb::create("tree setup", bind(&VRTree::storeSetup, this, placeholders::_1)) );
}

VRTree::~VRTree() {}
VRTreePtr VRTree::create(string name) { return shared_ptr<VRTree>(new VRTree(name)); }
VRTreePtr VRTree::ptr() { return static_pointer_cast<VRTree>( shared_from_this() ); }

void VRTree::storeSetup(VRStorageContextPtr context) {
    grow(seed);
    for (auto lp : foliage) growLeafs(lp);
}

void VRTree::initLOD() {
    /*lod = VRLod::create("tree_lod");
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
    lod->addDistance(50);*/
}

VRGeometryPtr VRTree::getLOD(int lvl) { return lod; }

float VRTree::random (float min, float max) {
    if (max!=min) {
        float c = 1e5;
        return (rand()%(int)(c*max-c*min)+c*min)/c;
    } else return max;
}

float VRTree::variation(float val, float var) { return random(val*(1-var), val*(1+var)); }

Vec3d VRTree::randUVec() { return Vec3d(random(-1,1), random(-1,1), random(-1,1)); }

VRMaterialPtr VRTree::treeMat = 0;
VRMaterialPtr VRTree::leafMat = 0;

//rotate a vector with angle 'a' in a random direction
Vec3d VRTree::randomRotate(Vec3d v, float a) {
    if (a == 0) return v;

    Vec3d x, d;

    do x = randUVec();
    while (x.dot(v) > 1e-3);

    d = v.cross(x);
    d.normalize();

    Quaterniond q(d, a);
    q.multVec(v, v);
    return v;
}

void VRTree::grow(int seed) { growSegment(seed); }

segment* VRTree::growSegment(int seed, segment* p, int iteration, float t) {
    this->seed = seed;
    if (int(parameters.size()) <= iteration) return 0;
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
        Vec3d d = p->p2 - p->p1; d.normalize();
        s->p1 = p->p1 + (p->p2-p->p1)*t;
        s->p2 = s->p1 + randomRotate(d, a)*l;

        s->n2 = s->p2 - s->p1;
        s->n2.normalize();
        s->n1 = p->n2 + (p->n2 - s->n2)*variation(sp->n_angle, sp->n_angle_var);
    } else {
        s->p2 *= variation(sp->length, sp->length_var);
    }

    for (int n=0; n<sp->nodes; n++) {
        for (int i=0; i<sp->child_number; i++) {
            auto c = growSegment(seed, s, iteration+1, (n+1)*1.0/sp->nodes);
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

VRMaterialPtr VRTree::getTruncMaterial() { return treeMat; }
VRMaterialPtr VRTree::getLeafMaterial() { return leafMat; }

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
        tg.add("Perlin", 1, truncColor, Color4f(1,0.9,0.7, 1));
        tg.add("Perlin", 0.25, Color4f(1,0.9,0.7, 1), truncColor);
        treeMat->setTexture(tg.compose(0));
    }

    if (!leafMat) {
        leafMat = VRMaterial::create("tree_leafs");

        leafMat->setDiffuse(Color3f(0.6,0.8,0.4));
        leafMat->setAmbient(Color3f(0.2, 0.6, 0.2));
        leafMat->setSpecular(Color3f(0.1, 0.1, 0.1));
        //leafMat->setPointSize(5); // try to increase shadow size, but doesnt work

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
    //if (!lod) initLOD();

    VRGeoData geo;
    for (segment* s : branches) {
        geo.pushVert(s->p1, s->n1, truncColor, Vec2d(s->radii[0]));
        geo.pushVert(s->p2, s->n2, truncColor, Vec2d(s->radii[1]));
        geo.pushLine();
    }

    auto g = geo.asGeometry("wood2");
    g->setPatchVertices(2);
    g->setType(GL_PATCHES);
    g->setMaterial(treeMat);
    addChild(g);
    woodGeos.push_back( g );
    //lod->getChild(i)->addChild(g);
}

VRObjectPtr VRTree::copy(vector<VRObjectPtr> children) {
    auto tree = VRTree::create();
    tree->trunc = trunc;
    tree->lod = lod;
    tree->branches = branches;
    tree->truncColor = truncColor;
    tree->woodGeos = woodGeos;
    tree->leafGeos = leafGeos;
    tree->leafLodCache = leafLodCache;
    return tree;
}

void VRTree::testSetup() {
    parameters.push_back( seg_params::create() );
    grow(time(0));
}

void VRTree::setup(int branching, int iterations, int seed, Vec4d params, Vec4d params_v) {
    for (int i=0; i<iterations; i++) {
        Vec4d params2 = params;
        params2[3] = 0.1*pow(params[3],i);
        addBranching(1, branching, params2, params_v);
    }

    grow(seed);
}

void VRTree::addBranching(int nodes, int branching, Vec4d params, Vec4d params_v) {
    auto sp = seg_params::create();
    sp->nodes = nodes;
    sp->child_number = branching;
    sp->n_angle = params[0];
    sp->p_angle = params[1];
    sp->length = params[2];
    sp->radius = params[3];
    sp->n_angle_var = params_v[0];
    sp->p_angle_var = params_v[1];
    sp->length_var = params_v[2];
    sp->radius_var = params_v[3];
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
    //if (!lod) initLOD();

    if (leafGeos.size() == 0) {
        auto g = VRGeometry::create("branches");
        g->setPersistency(0);
        leafGeos.push_back( g );
        addChild(g);
        //lod->getChild(i)->addChild(g);
        g->setMaterial(leafMat);
    }

    mt19937 e2(seed);
    normal_distribution<> ndist(0,1);

    auto randVecInSphere = [&](float r) {
        return Vec3d(ndist(e2), ndist(e2), ndist(e2))*r;
    };

    float ca = 1.0; // carotene
    float ch = 1.0; // chlorophyl
    ca = 0.5 + rand()*0.5/RAND_MAX;
    ch = 0.5 + rand()*0.5/RAND_MAX;
    VRGeoData geo(leafGeos[0]);

    for (auto b : branches) {
        if (b->lvl != lp->level) continue;

        // compute branch segment basis
        Vec3d p = (b->p1 + b->p2)*0.5;
        Vec3d d = b->p2 - b->p1;
        Vec3d u = Vec3d(0,1,0);
        Vec3d n = u.cross(d); n.normalize();
        u = d.cross(n); u.normalize();

        float L = d.length();

        for (int i=0; i<lp->amount; i++) {
            Vec3d v = randVecInSphere(L*0.3);
            Vec3d n = p+v-b->p1;
            n.normalize();
            // TODO: add model for carotene and chlorophyl depending on leaf age/size and more..
            geo.pushVert(p+v, n, Color3f(lp->size,ca,ch)); // color: leaf size, amount of carotene, amount of chlorophyl
            geo.pushPoint();
        }
    }

    if (geo.size() == 0) { cout << "VRTree::addLeafs Warning: no armature with level " << lp->level << endl; return; }

    /*for (int i=0; i<geo.size(); i++) {
        auto c = geo.getColor3(i); //c[0] *= 2; // double lod leaf size
        geo1.pushVert( geo.getPosition(i), geo.getNormal(i), c);
        geo1.pushPoint();
    }

    for (int i=0; i<geo.size(); i++) {
        auto c = geo.getColor3(i); //c[0] *= 4; // double lod leaf size
        geo2.pushVert( geo.getPosition(i), geo.getNormal(i), c);
        geo2.pushPoint();
    }*/

    geo.apply(leafGeos[0]);
}

void VRTree::setLeafMaterial(VRMaterialPtr mat) {
    for (auto g : leafGeos) g->setMaterial(mat);
}

string VRTree::treeSprLODvp =
"#version 120\n"
GLSL(
attribute vec4 osg_Vertex;
attribute vec3 osg_Normal;
attribute vec2 osg_MultiTexCoord0;
varying vec4 vertPos;
varying vec3 vertNorm;
varying vec4 color;
varying float Discard;

void main(void) {
	vertPos = gl_ModelViewMatrix * osg_Vertex;
	vertNorm = normalize( gl_NormalMatrix * osg_Normal );
	gl_TexCoord[0] = vec4(osg_MultiTexCoord0,0.0,0.0);
	color = gl_Color;

	//vec4 mid = normalize( gl_ModelViewMatrix * vec4(0,0,0,1) );
	float angle = dot(vec3(0,0,1), vertNorm);
	Discard = 0;
	if (abs(angle) < 0.6) Discard = 1; // discard depending on orientation
	vec4 p = gl_ModelViewProjectionMatrix*osg_Vertex;
	if (p[2] < 15) Discard = 1; // LoD
    gl_Position = p;
}
);

string VRTree::treeSprLODdfp =
"#version 120\n"
GLSL(
uniform int isLit;
varying vec4 vertPos;
varying vec3 vertNorm;
varying vec4 color;
uniform sampler2D tex0;
uniform sampler2D tex1;
varying float Discard;

void main(void) {
	if (Discard > 0.5) discard;
	vec3 pos = vertPos.xyz / vertPos.w;
	vec4 color = texture2D(tex0, gl_TexCoord[0].xy);
	vec4 normal = texture2D(tex1, gl_TexCoord[0].xy);
	normal.xyz = ( normal.xyz - vec3(0.5,0.5,0.5) )*2; // unpack RGB
	if (vertNorm.z > 0) normal.x *= -1;
	vec3 norm = normalize( normal.xyz );
	if (color.a < 0.3) discard;
	gl_FragData[0] = vec4(pos, 1.0);
	gl_FragData[1] = vec4(norm, isLit);
	gl_FragData[2] = vec4(color.rgb, 0);
}
);

void VRTree::appendLOD(VRGeoData& data, int lvl, Vec3d offset) {
    if (!lod) lod = createLOD(lvl);
    auto p = lod->getFrom();
    lod->setWorldPosition(offset);
    data.append(lod, lod->getWorldMatrix());
    lod->setFrom(p);
}

string VRTree::getHash(vector<float> v) {
    vector<int> data;
    data.push_back(seed);

    int k = 1e4;

    for (auto param : parameters) {
        data.push_back(param->nodes*k);
        data.push_back(param->child_number*k);
        data.push_back(param->n_angle*k);
        data.push_back(param->p_angle*k);
        data.push_back(param->length*k);
        data.push_back(param->radius*k);
        data.push_back(param->n_angle_var*k);
        data.push_back(param->p_angle_var*k);
        data.push_back(param->length_var*k);
        data.push_back(param->radius_var*k);
    }

    for (auto param : foliage) {
        data.push_back(param->level*k);
        data.push_back(param->amount*k);
        data.push_back(param->size*k);
    }

    for (auto f : v) data.push_back(f*k);

    size_t hash = 0;
    for (auto d : data) boost::hash_combine(hash, d);
    return toString( hash );
}

string channelDiffuseLeafFP =
"#version 400 compatibility\n"
GLSL(
uniform sampler2D tex;
in vec4 position;
in vec2 tcs;
in vec3 norm;
in vec3 col;
vec4 color;

void main( void ) {
	float ca = col[1];
	float ch = col[2];
	color = texture(tex,tcs);
	//if (color.a < 0.9) discard;

	color.x *= 0.4*ca;
	color.y *= 0.8*ch;
	color.z *= 0.2*ch;
	//gl_FragColor = vec4(color.xyz,1.0);
	gl_FragColor = vec4(color.xyz,color.a);
}
);

string channelNormalLeafFP =
"#version 400 compatibility\n"
GLSL(
uniform sampler2D tex;
in vec4 position;
in vec2 tcs;
in vec3 norm;
in vec3 col;
vec4 color;

void main( void ) {
	float ca = col[1];
	float ch = col[2];
	color = texture(tex,tcs);
	if (color.a < 0.3) discard;

	vec3 n = normalize( gl_NormalMatrix * norm );
	n = n*0.5 + vec3(0.5,0.5,0.5); // pack in RGB
	gl_FragColor = vec4(n,1.0);
}
);

vector<VRMaterialPtr> VRTree::createLODtextures(int& Hmax, VRGeoData& data) {
    auto storeSprite = [&](string path, VRMaterialPtr m) {
        auto t1 = m->getTexture(0);
        auto t2 = m->getTexture(1);
        t1->write(path+"_1.png");
        t2->write(path+"_2.png");
    };

    auto loadSprite = [&](string path, VRMaterialPtr m) -> bool {
        if (!exists(path+"_1.png") || !exists(path+"_2.png")) return false;
        m->setTexture(path+"_1.png", 1, 0);
        m->setTexture(path+"_2.png", 1, 1);
        return true;
    };

    VRTreePtr t = ptr();
    auto old_pose = t->getPose();
    t->setIdentity();
    auto bb = t->getBoundingbox();
    Vec3d S = bb->size();
    float h2 = S[1]*0.5;
    float fov = 0.33;
    Vec3d D = S*0.5/tan(fov*0.5);

    // setup material substitues for lod render pass for normal and diffuse passes
    map<VRMaterial*, VRMaterialPtr> matSubsDiff;
    map<VRMaterial*, VRMaterialPtr> matSubsNorm;

    auto leafMatDiff = dynamic_pointer_cast<VRMaterial>( leafMat->duplicate() );
    leafMatDiff->setFragmentShader(channelDiffuseLeafFP, "texRendChannel");
    matSubsDiff[leafMat.get()] = leafMatDiff;
    auto leafMatNorm = dynamic_pointer_cast<VRMaterial>( leafMat->duplicate() );
    leafMatNorm->setFragmentShader(channelNormalLeafFP, "texRendChannel");
    matSubsNorm[leafMat.get()] = leafMatNorm;

    string wdir = VRSceneManager::get()->getOriginalWorkdir();

    auto treeMatDiff = dynamic_pointer_cast<VRMaterial>( treeMat->duplicate() );
    treeMatDiff->readFragmentShader(wdir+"/shader/Trees/Shader_tree_channel_diffuse.fp");
    matSubsDiff[treeMat.get()] = treeMatDiff;
    auto treeMatNorm = dynamic_pointer_cast<VRMaterial>( treeMat->duplicate() );
    treeMatNorm->readFragmentShader(wdir+"/shader/Trees/Shader_tree_channel_normal.fp");
    matSubsNorm[treeMat.get()] = treeMatNorm;

    vector<VRMaterialPtr> sides;
    Hmax = 1;
    auto addSprite = [&](PosePtr p, float W, float H, float Sh) {
        string path = ".treeLods/treeLod"+getHash({W,H,Sh});
        VRMaterialPtr tm = VRMaterial::create("lod");
        if (!loadSprite(path, tm)) {
            auto tr = VRTextureRenderer::create("treeLODtexR");
            tr->setBackground(Color3f(0,0,0), 0);
            tr->setMaterialSubstitutes(matSubsDiff, VRTextureRenderer::DIFFUSE);
            tr->setMaterialSubstitutes(matSubsNorm, VRTextureRenderer::NORMAL);
            tm = tr->createTextureLod(t, p, 512, W/H, fov, Color3f(0,0.5,0));
            storeSprite(path, tm);
        }
        int h = int(512/W*H);
        Hmax = max(Hmax, h);
        sides.push_back(tm);
        data.pushQuad(Vec3d(0,Sh,0), p->dir(), p->up(), Vec2d(W, H), true);
    };

    addSprite( Pose::create(Vec3d(0,h2,-D[1]), Vec3d(0,0,1), Vec3d(0,1,0)), S[0], S[1], S[1]*0.5);
    addSprite( Pose::create(Vec3d(-D[1],h2,0), Vec3d(1,0,0), Vec3d(0,1,0)), S[2], S[1], S[1]*0.5);
    addSprite( Pose::create(Vec3d(0,S[1]+max(D[0],D[2]),0), Vec3d(0,-1,0), Vec3d(0,0,1)), S[0], S[2], S[1]);

    t->setPose(old_pose);
    return sides;
}

vector<VRMaterialPtr> VRTree::getLodMaterials() { return lodMaterials; }

VRGeometryPtr VRTree::createLOD(int lvl) {
    VRGeoData data;
    int Hmax = 1;
    auto sides = createLODtextures(Hmax, data);
    lodMaterials = sides;
    int N = sides.size();
    if (N == 0) return 0;

    auto mosaic1 = VRTextureMosaic::create();
    auto mosaic2 = VRTextureMosaic::create();
    for (int i=0; i<N; i++) {
        mosaic1->add( sides[i]->getTexture(0), Vec2i(512*i,0), Vec2i(i,0) );
        mosaic2->add( sides[i]->getTexture(1), Vec2i(512*i,0), Vec2i(i,0) );
    }
    auto m = VRMaterial::create("treeLOD");
    m->setTexture(mosaic1, false, 0);
    m->setTexture(mosaic2, false, 1);

    for (int i=0; i<N; i++) { // create UV coordinates
        float i1 = i*1.0/N;
        float i2 = (i+1)*1.0/N;
        float h = sides[i]->getTexture(0)->getSize()[1] / float(Hmax);
        data.setTexCoord(4*i  , Vec2d(i2,0));
        data.setTexCoord(4*i+1, Vec2d(i1,0));
        data.setTexCoord(4*i+2, Vec2d(i1,h));
        data.setTexCoord(4*i+3, Vec2d(i2,h));
    }

    m->setShaderParameter("tex0", 0);
    m->setShaderParameter("tex1", 1);
    m->setVertexShader(treeSprLODvp, "treeSprLODvp");
    m->setFragmentShader(treeSprLODdfp, "treeSprLODdfp", true);

    auto geo = data.asGeometry("treeLod");
    geo->setMaterial(m);
    lod = geo;
    return geo;
}

void VRTree::createTwigLod(VRGeoData& geo, int lvl) {
    ;
}

void VRTree::createHullLeafLod(VRGeoData& geo, int lvl, Vec3d offset, int ID) {
    Matrix4d Offset;
    Offset.setTranslate(offset); // TODO, use tree transformation rotation?

    if (leafLodCache.count(lvl)) {
        geo.append(leafLodCache[lvl], Offset);
        return;
    }

    if (leafGeos.size() == 0) return;
    VRGeoData data(leafGeos[0]);

    auto computeHull = [&](VRGeoData& tmpData, Color4f color) -> VRGeometryPtr {
        if (tmpData.size() == 0) return 0;
        float ca = color[1]; // carotene
        float ch = color[2]; // chlorophyll
        Color3f leafColor(0.4*ca,0.8*ch,0.2*ch);
#ifndef WITHOUT_LAPACKE_BLAS
        VRConvexHull hull;
        auto hgeo = hull.compute( tmpData.asGeometry("tmpdata") );
        if (!hgeo) return 0;
        auto res = VRGeoData( hgeo );
        for (int i=0; i<res.size(); i++) res.pushColor( leafColor );
        return res.size() > 0 ? res.asGeometry("tmp") : 0;
#else
        return 0;
#endif
    };

    int Ns = pow(2,int(4/(lvl+1)));
    vector<VRGeoData> clusters(Ns);
    vector<Pnt3d> seeds(Ns);
    for (int k=0; k<Ns; k++) {
        int p = data.size()*k*1.0/Ns;
        seeds[k] = data.getPosition(p);
    }

    auto getMinCluster = [&](const Pnt3d& pos) -> VRGeoData& {
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

    int N = 100/lvl;
    float D = float(data.size())/N;
    float fuzzy = 0.2;

    random_device rd;
    mt19937 e2(rd());
    normal_distribution<> ndist(0,fuzzy);
    auto randVecInSphere = [&]() {
        return Vec3d(ndist(e2), ndist(e2), ndist(e2));
    };

    Color4f meanColor;
    for (int i=0; i<N; i++) {
        int j = max( min( int(i*D), data.size()-1), 0);
        meanColor += data.getColor(j);
        Pnt3d pos = data.getPosition(j);
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

void VRTree::createHullTrunkLod(VRGeoData& geo, int lvl, Vec3d offset, int ID) {
    Matrix4d Offset;
    Offset.setTranslate(offset); // TODO, use tree transformation rotation?

    Vec2d id = Vec2d(ID,1); // the 1 is a flag to identify the ID as such!

    if (truncLodCache.count(lvl)) {
        geo.append(truncLodCache[lvl], Offset);
        return;
    }

    if (!trunc) return;

    VRGeoData Hull;

    auto normalize = [](Vec3d v) {
        v.normalize();
        return v;
    };

    auto pushRing = [&](Vec3d p, float r) {
        static Vec3d n1 = normalize( Vec3d(-1,0,-1) );
        static Vec3d n2 = normalize( Vec3d(-1,0, 1) );
        static Vec3d n3 = normalize( Vec3d( 1,0, 1) );
        static Vec3d n4 = normalize( Vec3d( 1,0,-1) );
        int i1 = Hull.pushVert( Pnt3d(-r,0,-r) + p, n1, truncColor, id );
        int i2 = Hull.pushVert( Pnt3d(-r,0, r) + p, n2, truncColor, id );
        int i3 = Hull.pushVert( Pnt3d( r,0, r) + p, n3, truncColor, id );
        int i4 = Hull.pushVert( Pnt3d( r,0,-r) + p, n4, truncColor, id );
        return Vec4i(i1,i2,i3,i4);
    };

    auto pushBox = [&](Vec4i i1, Vec4i i2) {
        Hull.pushQuad(i1[0],i1[1],i2[1],i2[0]);
        Hull.pushQuad(i1[1],i1[2],i2[2],i2[1]);
        Hull.pushQuad(i1[2],i1[3],i2[3],i2[2]);
        Hull.pushQuad(i1[3],i1[0],i2[0],i2[3]);
    };

    auto pushCross = [&](Vec4i i1, Vec4i i2) {
        Hull.pushQuad(i1[0],i1[2],i2[2],i2[0]);
        Hull.pushQuad(i1[1],i1[3],i2[3],i2[1]);
    };

    int Nlvl = 3;
    if (lvl >= 2) Nlvl = 2;
    if (lvl >= 3) Nlvl = 1;
    if (lvl >= 4) Nlvl = 0;
    for (segment* s : branches) {
        if (s->lvl > Nlvl) continue;
        auto i0 = pushRing(s->p1, s->radii[0]);
        auto i1 = pushRing(s->p2, s->radii[1]);
        if (Nlvl <= 1) pushCross(i0,i1);
        else pushBox(i0,i1);
    }

    truncLodCache[lvl] = Hull.asGeometry("truncLodCache");
    geo.append(Hull, Offset);
}






