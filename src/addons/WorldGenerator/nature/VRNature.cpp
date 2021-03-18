#include "VRNature.h"
#include "VRTree.h"
#include "VRGrassPatch.h"
#include "../terrain/VRTerrain.h"
#include "../VRWorldGenerator.h"

#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#ifndef WITHOUT_BULLET
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRSpatialCollisionManager.h"
#endif
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/material/VRTextureMosaic.h"
#include "core/objects/VRLod.h"
#include "core/objects/VRLodTree.h"
#include "core/math/Octree.h"
#include "core/math/pose.h"
#include "core/math/polygon.h"
#include "core/math/boundingbox.h"
#ifndef WITHOUT_GLU_TESS
#include "core/math/triangulator.h"
#endif
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/utils/VRTimer.h"

using namespace OSG;

// --------------------------------------------------------------------------------------------------

VRNature::VRNature(string name) {
    trees = VRGeometry::create("trees");
    trees->hide("SHADOW");

    storeMap("templateTrees", &treeTemplates, true);
    storeMap("trees", &treeEntries, true);
    regStorageSetupFkt( VRStorageCb::create("woods setup", bind(&VRNature::setup, this, placeholders::_1)) );
}

VRNature::~VRNature() {}
VRNaturePtr VRNature::ptr() { return static_pointer_cast<VRNature>( shared_from_this() ); }
VRNaturePtr VRNature::create(string name) {
    auto nat = VRNaturePtr(new VRNature(name));
    nat->addChild(nat->lodTree);
    nat->addChild(nat->trees);
    return nat;
}

void VRNature::setup(VRStorageContextPtr context) {
    for (auto& t : treeEntries) {
        if (!treeTemplates.count(t.second->type)) { cout << "VRNature::setup Warning, " << t.second->type << " is not a tree template!" << endl; continue; }
        auto tree = treeTemplates[t.second->type];
        tree->setPose(t.second->pos);
        addTree(tree, 0, false);
    }
    computeAllLODs();
}

VRTreePtr VRNature::getTree(int id) {
    if (treesByID.count(id)) return treesByID[id];
    return 0;
}

template<typename T>
string getRandomKey(T& map) {
    if (map.size() == 0) return "";
    int i = rand()%map.size();
    auto itr = map.begin();
    advance(itr, i);
    return itr->first;
}

VRTreePtr VRNature::createRandomTree(Vec3d p) { return createTree( getRandomKey(treeTemplates), p ); }
VRTreePtr VRNature::createRandomBush(Vec3d p) { return createBush( getRandomKey(bushTemplates), p ); }
VRLodTreePtr VRNature::getLodTree() { return lodTree; }

VRTreePtr VRNature::createTree(string type, Vec3d p) {
    if (!treeTemplates.count(type)) return 0;
    auto t = dynamic_pointer_cast<VRTree>(treeTemplates[type]->duplicate());
    if (!t) return 0;
    t->addAttachment("tree", 0);
    if (auto t = terrain.lock()) p = t->elevatePoint(p);
    t->setFrom(p);
    lodTree->addObject(t, p, 0);
    treeRefs[t.get()] = treeTemplates[type];
    treesByID[t->getID()] = t;
    return t;
}

VRTreePtr VRNature::createBush(string type, Vec3d p) {
    //cout << " - - - VRNature::createBush " << type << " p " << p << endl;
    if (!bushTemplates.count(type)) return 0;
    auto t = dynamic_pointer_cast<VRTree>(bushTemplates[type]->duplicate());
    if (!t) return 0;
    t->addAttachment("tree", 0);
    if (auto t = terrain.lock()) p = t->elevatePoint(p);
    t->setFrom(p);
    lodTree->addObject(t, p, 0);
    treeRefs[t.get()] = bushTemplates[type];
    treesByID[t->getID()] = t;
    return t;
}

void VRNature::simpleInit(int treeTypes, int bushTypes) {
    auto doTree = [&]() {
		float H = 2+rand()*6.0/RAND_MAX;
		int Nn = int(H)-1;
		float r0 = 0.05+rand()*0.15/RAND_MAX;
		float l0 = H/Nn;
		auto t = VRTree::create("tree");
		Vec4d params_v = Vec4d(0.2,0.4,0.2,0.2);
		t->addBranching(Nn,4, Vec4d(0.2,0.4,H,r0), params_v);
		t->addBranching(1,4, Vec4d(0.2,0.4,0.8*l0,r0*pow(0.3,1)), params_v);
		t->addBranching(1,4, Vec4d(0.2,0.4,0.7*l0,r0*pow(0.3,2)), params_v);
		t->addBranching(1,4, Vec4d(0.2,0.4,0.6*l0,r0*pow(0.3,3)), params_v);
		t->addBranching(1,4, Vec4d(0.2,0.4,0.5*l0,r0*pow(0.3,4)), params_v);
		t->grow( int(rand()*100.0/RAND_MAX) );
		t->addLeafs(4, 10, 0.1); // int lvl, int amount, float size
		return t;
    };

    auto doBush = [&]() {
		auto t = VRTree::create("bush");
		Vec4d params_v = Vec4d(0.2,0.4,0.2,0.2);
		t->addBranching(1,10, Vec4d(0.2,0.4,0.1,0.01), params_v);
		t->addBranching(1,4, Vec4d(0.2,0.9,0.3,0.006), params_v);
		t->addBranching(1,4, Vec4d(0.2,0.4,0.2,0.004), params_v);
		//t->addBranching(1,4, Vec4d(0.2,0.4,0.2,0.0004), params_v);
		t->grow( int(rand()*100.0/RAND_MAX) );
		t->addLeafs(2, 4, 0.1);
		return t;
    };

    for (int i=0; i<treeTypes; i++) addTreeTemplate( doTree() );
    for (int i=0; i<bushTypes; i++) addBushTemplate( doBush() );
}

void VRNature::addTreeTemplate(VRTreePtr t) { treeTemplates[t->getName()] = t; t->createLOD(0); }
void VRNature::addBushTemplate(VRTreePtr t) { bushTemplates[t->getName()] = t; t->createLOD(0); }

void VRNature::removeTree(int id) {
    if (!treesByID.count(id)) return;
    auto t = treesByID[id];
    auto leaf = lodTree->remObject(t);
    treesByID.erase(id);
    treeRefs.erase(t.get());
    treeEntries.erase(t->getName());
    t->destroy();

    auto oLeafs = leaf->getOLeaf()->getAncestry();
    map<OctreeNode*, VRLodLeafPtr> aLeafs;
    for (auto o : oLeafs) {
        auto l = lodTree->getLeaf(o);
        if (l) aLeafs[o] = l;
    }
    computeLODs(aLeafs);
}

VRGeometryPtr VRNature::getCollisionObject() { return collisionMesh; }

void VRNature::addScrub(VRPolygonPtr area, bool addGround) {
    float a = area->computeArea();
    if (a == 0) return;

    //VRTimer timer; timer.start();
    //int t0 = timer.stop();

    if (auto t = terrain.lock()) t->elevatePolygon(area, 0.18);
    Vec3d median = area->getBoundingBox().center();
    area->translate(-median);
    for (auto p : area->getRandomPoints(1.0, -0.1, 0.7)) createRandomBush(median+p);

#ifndef WITHOUT_GLU_TESS
    if (addGround) {
        Triangulator tri;
        tri.add(*area);
        auto ground = tri.compute();
        ground->translate(median);
        ground->setPositionalTexCoords(1.0, 0, Vec3i(0,2,1));
        ground->updateNormals();
        ground->flipNormals();

        if (!groundPatches) {
            groundPatches = VRGeometry::create("groundPatches");
            addChild( groundPatches );
            VRTextureGenerator tg;
            tg.addSimpleNoise( Vec3i(128,128,1), false, Color4f(0.85,0.8,0.75,1), Color4f(0.5,0.3,0,1) );
            auto mat = VRMaterial::create("earth");
            mat->setTexture(tg.compose());
            groundPatches->setMaterial(mat);
        }
        groundPatches->merge(ground, ground->getPose());
        groundPatches->setPositionalTexCoords(1.0, 0, Vec3i(0,2,1)); // TODO: fix issues in VRGeoData
    }
#endif
}

void VRNature::addGrassPatch(VRPolygonPtr Area, bool updateLODs, bool addGround) { // TODO: needs optimizations!
    //VRTimer timer; timer.start();
    //int t0 = timer.stop();
    //cout << "VRNature::addGrassPatch " << t0 << endl;
    int i=0;
    auto ground = VRGeometry::create("ground");

    float a = Area->computeArea();
    if (a == 0) return;
    //cout << "VRNature::addGrassPatch " << a << endl;

    map<VRLodLeafPtr, bool> toUpdate;

    for (auto area : Area->gridSplit(10.0)) {
        if (area->isCCW()) area->reverseOrder();
        //cout << " sub Area " << i << "  " << timer.stop() - t0 << endl;
        if (auto t = terrain.lock()) t->elevatePolygon(area, 0.18);
        Vec3d median = area->getBoundingBox().center();
        area->translate(-median);
        //cout << "  A1 " << timer.stop() - t0 << endl;
        auto grass = VRGrassPatch::create();
        grass->addAttachment("grass", 0);
        grass->setArea(area);
        //cout << "  A2 " << timer.stop() - t0 << endl;
        grassPatchRefs[grass.get()] = grass;
        auto leaf = lodTree->addObject(grass, median, 0); // pose contains the world position!
        grass->setWorldPosition(median);
        toUpdate[leaf] = true;


        //cout << "  A3 " << timer.stop() - t0 << endl;

        //cout << " VRNature::addGrassPatch " << median << "   " << area->computeArea() << endl;

#ifndef WITHOUT_GLU_TESS
        if (addGround) {
            Triangulator tri;
            tri.add(*area);
            auto geo = tri.compute();
            geo->translate(median);
            ground->merge(geo);
        }
#endif

        i++;
    }

    if (updateLODs) for (auto l : toUpdate) computeLODs(l.first);

    if (addGround) {
        VRTextureGenerator tg;
        tg.addSimpleNoise( Vec3i(128,128,1), true, Color4f(0.85,0.8,0.75,1), Color4f(0.5,0.3,0,1) );
        auto mat = VRMaterial::create("earth");
        mat->setTexture(tg.compose());
        mat->clearTransparency();
        ground->setPositionalTexCoords(1.0, 0, Vec3i(0,2,1));
        //addChild(ground);
        if (!grassGroundPatches) {
            grassGroundPatches = VRGeometry::create("grassGroundPatches");
            addChild( grassGroundPatches );
            grassGroundPatches->setMaterial(mat);
        }
        grassGroundPatches->merge(ground);
    }
}

VRTreePtr VRNature::addTree(VRTreePtr t, bool updateLODs, bool addToStore) { // TODO: needs refactoring!!
    if (!t) return 0;
    PosePtr p = t->getRelativePose(ptr());
    if (auto t = terrain.lock()) t->elevatePose(p);

    auto tree = dynamic_pointer_cast<VRTree>( t->duplicate() );
    tree->addAttachment("tree", 0);
    treeRefs[tree.get()] = t;
    auto leaf = lodTree->addObject(tree, p->pos(), 0); // pose contains the world position!
    treesByID[tree->getID()] = tree;

    auto te = VRObjectManager::Entry::create();
    te->set( p, t->getName());
    if (addToStore) treeEntries[tree->getName()] = te;
    if (updateLODs) computeLODs(leaf);
    return tree;
}

VRTreePtr VRNature::addBush(VRTreePtr t, bool updateLODs, bool addToStore) {
    if (!t) return 0;
    PosePtr p = t->getRelativePose(ptr());
    if (auto t = terrain.lock()) t->elevatePose(p);

    auto tree = dynamic_pointer_cast<VRTree>( t->duplicate() );
    tree->addAttachment("tree", 0);
    bushTemplates[t->getName()] = t;
    treeRefs[tree.get()] = t;
    auto leaf = lodTree->addObject(tree, p->pos(), 0); // pose contains the world position!
    treesByID[tree->getID()] = tree;

    auto te = VRObjectManager::Entry::create();
    te->set( p, t->getName());
    if (addToStore) treeEntries[tree->getName()] = te;
    if (updateLODs) computeLODs(leaf);
    return tree;
}

void VRNature::computeLODsThread(VRThreadWeakPtr tw) {
    //if (jobs.size() == 0) { sleep(1); return; } // start as loop
    //MapManager::job j = jobs.front(); jobs.pop_front();

    VRThreadPtr t = tw.lock();
    t->syncFromMain();
    computeLODs3(lodTree->getLeafs());
    t->syncToMain();
}

void VRNature::computeAllLODs(bool threaded) {
    if (!threaded) { computeLODs3(lodTree->getLeafs()); return; }

    auto scene = VRScene::getCurrent();
    worker = VRThreadCb::create( "nature lods", bind(&VRNature::computeLODsThread, this, placeholders::_1) );
    scene->initThread(worker, "nature lods", false, 1);
}

void VRNature::computeLODs(VRLodLeafPtr leaf) {
    auto oLeafs = leaf->getOLeaf()->getAncestry();
    map<OctreeNode*, VRLodLeafPtr> aLeafs;
    for (auto o : oLeafs) {
        if (auto l = lodTree->getLeaf(o)) aLeafs[o] = l;
    }
    computeLODs(aLeafs);
}

void VRNature::computeLODs2(map<OctreeNode*, VRLodLeafPtr>& leafs) {

    // get all trees and grass patches for each leaf layer
    map<VRLodLeaf*, vector<VRTree*> > trees;
    map<VRLodLeaf*, vector<VRGrassPatch*> > grass;
    for (auto l : leafs) {
        auto& leaf = l.second;
        int lvl = leaf->getLevel();
        if (lvl == 0) continue;

        vector<void*> data = leaf->getOLeaf()->getAllData();
        for (auto v : data) {
            if (((VRObject*)v)->hasTag("tree")) trees[leaf.get()].push_back((VRTree*)v);
            if (((VRObject*)v)->hasTag("grass")) grass[leaf.get()].push_back((VRGrassPatch*)v);
        }
    }

    // create layer node geometries
    for (auto l : leafs) {
        auto& leaf = l.second;
        leaf->reset();
        int lvl = leaf->getLevel();
        if (lvl == 0) continue;
        bool doTrees = (trees.count(leaf.get()) >= 0);
        //bool doGrass = (grass.count(leaf.get()) >= 0);

        Boundingbox bb;
        if (doTrees) for (auto t : trees[leaf.get()]) bb.update( t->getWorldPosition() );
        //if (doGrass) for (auto g : grass[leaf.get()]) bb.update( g->getWorldPosition() );
        Vec3d pos = bb.center();

        VRGeoData geo;
        VRMaterialPtr m;

        if (doTrees) for (auto t : trees[leaf.get()]) {
            if (treeRefs.count(t) == 0) continue;
            auto tRef = treeRefs[t];
            if (!tRef || !t) continue;
            Vec3d offset = t->getWorldPosition() - pos;
            VRTransformPtr tlod = tRef->getLOD(0);
            /*tRef->appendLOD(geo, lvl, offset);
            m = tlod->getMaterial();*/
            VRTransformPtr l = dynamic_pointer_cast<VRTransform>( tlod->duplicate() );
            leaf->add( l, 1 );
            l->setWorldPosition(t->getWorldPosition());
        }

        /*if (doGrass) for (auto g : grass[leaf.get()]) {
            if (grassPatchRefs.count(g) == 0) continue;
            auto gRef = grassPatchRefs[g];
            if (!gRef || !g) continue;
            Vec3d offset = g->getWorldPosition() - pos;
            gRef->createLod(geoGrass, lvl, offset, g->getID());
        }*/

        /*VRGeometryPtr woods;
        if (geo.size() > 0) {
            woods = geo.asGeometry("woods");
            if (m) woods->setMaterial(m);
            //cout << "YAY: lod data: " << geo.size() << " " << woods << endl;

            leaf->set( woods, 1 );
            woods->setWorldPosition(pos);
            woods->setDir(Vec3d(0,0,-1));
            woods->setUp(Vec3d(0,1,0));
        }*/
    }
}

void VRNature::computeLODs3(map<OctreeNode*, VRLodLeafPtr>& leafs) {
    // construct master material
    auto mosaic1 = VRTextureMosaic::create();
    auto mosaic2 = VRTextureMosaic::create();

    int j = 0;
    int k = 0;
    int H = 0;

    auto prepSprites = [&]( map<string, VRTreePtr>& templates ) {
        for (auto tree : templates) {
            if (!tree.second) continue;

            int Hmax = 1;
            auto sides = tree.second->getLodMaterials();
            for (auto side : sides) Hmax = max(Hmax, side->getTexture(0)->getSize()[1]);

            for (unsigned int i=0; i<sides.size(); i++) {
                mosaic1->add( sides[i]->getTexture(0), Vec2i(512*i,H), Vec2i(i,j) );
                mosaic2->add( sides[i]->getTexture(1), Vec2i(512*i,H), Vec2i(i,j) );
                //sides[i]->getTexture(0)->write("test_"+toString(i)+".png");
            }

            H += Hmax;
            j++;
        }
    };

    auto updateUVs = [&]( map<string, VRTreePtr>& templates ) {
        for (auto tree : templates) {
            VRGeometryPtr tlod = dynamic_pointer_cast<VRGeometry>( tree.second->getLOD(0) );
            VRGeoData data(tlod);

            int N = 3;
            for (int i=0; i<N; i++) { // update UV coordinates
                Vec2i ID = Vec2i(i,k);
                float i1 = i*1.0/N;
                float i2 = (i+1)*1.0/N;
                float j1 = mosaic1->getChunkPosition(ID)[1]/float(H);
                float j2 = j1+mosaic1->getChunkSize(ID)[1]/float(H);
                data.setTexCoord(4*i  , Vec2d(i2,j1));
                data.setTexCoord(4*i+1, Vec2d(i1,j1));
                data.setTexCoord(4*i+2, Vec2d(i1,j2));
                data.setTexCoord(4*i+3, Vec2d(i2,j2));
            }
            k++;
        }
    };

    prepSprites(treeTemplates);
    prepSprites(bushTemplates);
    updateUVs(treeTemplates);
    updateUVs(bushTemplates);
    mosaic1->write("test.png");

    auto m = VRMaterial::create("natureMosaic");
    m->setVertexShader(VRTree::treeSprLODvp, "treeSprLODvp");
    m->setFragmentShader(VRTree::treeSprLODdfp, "treeSprLODdfp", true);
    m->setShaderParameter("tex0", 0);
    m->setShaderParameter("tex1", 1);
    m->setTexture(mosaic1, false, 0);
    m->setTexture(mosaic2, false, 1);
    trees->setMaterial(m);
    VRGeoData treesData(trees);

    cout << "treeRefs: " << treeRefs.size() << endl;
    for (auto tree : treeRefs) {
        auto tRef = tree.second;
        VRTree* t = tree.first;
        if (!tRef || !t) continue;
        Vec3d offset = t->getWorldPosition();
        VRTransformPtr tlod = tRef->getLOD(0);
        VRGeometryPtr l = dynamic_pointer_cast<VRGeometry>( tlod->duplicate() );
        VRGeoData other(l);
        treesData.append(other, getMatrixTo(t->ptr()) );
    }

    treesData.apply(trees);
}

void VRNature::computeLODs(map<OctreeNode*, VRLodLeafPtr>& leafs) {
    auto simpleLeafMat = [](bool doAlpha) {
        auto m = VRMaterial::create("simpleLeafMat");
        m->setPointSize(3);
        //m->setDiffuse(Color3f(0.5,1,0));
        m->setDiffuse(Color3f(0,0,1));
        m->setAmbient(Color3f(0.1,0.3,0));
        m->setSpecular(Color3f(0.1,0.4,0));
        string wdir = VRSceneManager::get()->getOriginalWorkdir();
        m->readFragmentShader(wdir+"/shader/Trees/Shader_leafs_lod.fp");
        m->readVertexShader(wdir+"/shader/Trees/Shader_leafs_lod.vp");

        auto tg = VRTextureGenerator::create();
        tg->setSize(Vec3i(50,50,50), doAlpha);
        float r = 0.85;
        float g = 1.0;
        float b = 0.8;
        tg->add(PERLIN, 1, Color4f(r,g,b,0.9), Color4f(r,g,b,1) );
        tg->add(PERLIN, 0.5, Color4f(r,g,b,0.5), Color4f(r,g,b,1) );
        tg->add(PERLIN, 0.25, Color4f(r,g,b,0.8), Color4f(r,g,b,1) );
        m->setTexture(tg->compose(0));

        return m;
    };

    auto simpleTrunkMat = []() {
        auto m = VRMaterial::create("brown");
        m->setDiffuse(Color3f(0.6,0.3,0));
        string wdir = VRSceneManager::get()->getOriginalWorkdir();
        m->readFragmentShader(wdir+"/shader/Trees/Shader_trunc_lod.fp");
        m->readVertexShader(wdir+"/shader/Trees/Shader_trunc_lod.vp");
        return m;
    };

    auto simpleGrassMat = []() {
        return VRGrassPatch::getGrassSideMaterial();
    };

    // get all trees and grass patches for each leaf layer
    map<VRLodLeaf*, vector<VRTree*> > trees;
    map<VRLodLeaf*, vector<VRGrassPatch*> > grass;
    for (auto l : leafs) {
        auto& leaf = l.second;
        int lvl = leaf->getLevel();
        if (lvl == 0) continue;

        vector<void*> data = leaf->getOLeaf()->getAllData();
        for (auto v : data) {
            if (((VRObject*)v)->hasTag("tree")) trees[leaf.get()].push_back((VRTree*)v);
            if (((VRObject*)v)->hasTag("grass")) grass[leaf.get()].push_back((VRGrassPatch*)v);
        }
    }

    // create layer node geometries
    for (auto l : leafs) {
        auto& leaf = l.second;
        leaf->reset();
        int lvl = leaf->getLevel();
        if (lvl == 0) continue;
        bool doTrees = (trees.count(leaf.get()) >= 0);
        bool doGrass = (grass.count(leaf.get()) >= 0);

        Boundingbox bb;
        if (doTrees) for (auto t : trees[leaf.get()]) bb.update( t->getWorldPosition() );
        //if (doGrass) for (auto g : grass[leaf.get()]) bb.update( g->getWorldPosition() );
        Vec3d pos = bb.center();

        VRGeoData geoLeafs;
        VRGeoData geoTrunk;
        VRGeoData geoGrass;

        if (doTrees) for (auto t : trees[leaf.get()]) {
            if (treeRefs.count(t) == 0) continue;
            auto tRef = treeRefs[t];
            if (!tRef || !t) continue;
            Vec3d offset = t->getWorldPosition() - pos;
            tRef->createHullTrunkLod(geoTrunk, lvl, offset, t->getID());
            tRef->createHullLeafLod (geoLeafs, lvl, offset, t->getID());
        }

        if (doGrass) for (auto g : grass[leaf.get()]) {
            if (grassPatchRefs.count(g) == 0) continue;
            auto gRef = grassPatchRefs[g];
            if (!gRef || !g) continue;
            Vec3d offset = g->getWorldPosition() - pos;
            gRef->createLod(geoGrass, lvl, offset, g->getID());
        }

        VRGeometryPtr trunk;
        if (geoTrunk.size() > 0) {
            trunk = geoTrunk.asGeometry("trunk");
            if (!truncMat) truncMat = simpleTrunkMat();
            trunk->setMaterial(truncMat);

            leaf->set( trunk, 1 );
            trunk->setWorldPosition(pos);
            trunk->setDir(Vec3d(0,0,-1));
            trunk->setUp(Vec3d(0,1,0));

            if (geoLeafs.size() > 0) {
                auto leafs = geoLeafs.asGeometry("leafs");
                leafs->setPersistency(0);
                trunk->addChild( leafs );
                if (!leafMat1) leafMat1 = simpleLeafMat(true);
                if (!leafMat2) leafMat2 = simpleLeafMat(false);
                if (lvl < 3) leafs->setMaterial(leafMat1);
                else leafs->setMaterial(leafMat2);
            }
        }

        if (geoGrass.size() > 0) {
            auto patch = geoGrass.asGeometry("grass");
            if (!grassMat) grassMat = simpleGrassMat();
            patch->setMaterial(grassMat);

            if (trunk) trunk->addChild(patch);
            else leaf->set( patch, 1 );
            patch->setWorldPosition(pos);
            patch->setDir(Vec3d(0,0,-1));
            patch->setUp(Vec3d(0,1,0));
        }
    }
}

void VRNature::clear() {
    treesByID.clear();
    treeRefs.clear();
    treeEntries.clear();
    treeTemplates.clear();
    lodTree->reset();
}

void VRNature::addCollisionModels() {
    VRGeoData data;
    for (auto tree : treesByID) {
        auto p = getPoseTo( tree.second );
        data.pushQuad(p->pos()+Vec3d(0,1,0), p->dir(), p->up(), Vec2d(0.3, 2), true);
    }

#ifndef WITHOUT_BULLET
    auto geo = data.asGeometry("natureCollisionMesh");
	if (auto w = world.lock()) w->getPhysicsSystem()->add(geo, trees->getID());
#endif
}

/**

TODO:
 - optimize level zero structure

*/



