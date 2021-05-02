#include "Quadtree.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <time.h>
#include <sstream> //for std::stringstream
#include <string>  //for std::string

#include "core/utils/toString.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"

using namespace OSG;

QuadtreeNode::QuadtreeNode(QuadtreePtr tree, float res, float s, int lvl) : PartitiontreeNode(res, s, lvl) { this->tree = tree; }
QuadtreeNode::~QuadtreeNode() {
    for (auto c : children) if (c) delete c;
}

QuadtreeNode* QuadtreeNode::get(Vec3d p, bool checkPosition) {
    if ( !inBox(p, center, size) && checkPosition ) {
        if (parent) return parent->get(p, true);
        else return 0;
    }

    if (size > resolution) {
        int o = getOctant(p);
        if (!children[o]) return this;
        return children[o]->get(p, false);
    }

    return this;
}

// checkPosition avoids parent/child cycles due to float error
// partitionLimit sets a max amount of data points, tree is subdivided if necessary!

QuadtreeNode* QuadtreeNode::add(Vec3d pos, void* dat, int targetLevel, bool checkPosition, int partitionLimit) {
    Vec3d rpos = pos - center;

    auto createParent = [&]() {
        parent = new QuadtreeNode(tree.lock(), resolution, 2*size, level+1);
        parent->center = center + lvljumpCenter(size*0.5, rpos);
        int o = parent->getOctant(center);
        parent->children[o] = this;
        tree.lock()->updateRoot();
    };

    auto createChild = [&](int octant) {
        children[octant] = new QuadtreeNode(tree.lock(), resolution, size*0.5, level-1);
        Vec3d c = center + lvljumpCenter(size*0.25, rpos);
        children[octant]->center = c;
        children[octant]->parent = this;
    };

    auto reachedTargetLevel = [&]() {
        if (size <= resolution) return true;
        if (level == targetLevel && targetLevel != -1) return true;
        return false;
    };

    auto reachedPartitionLimit = [&]() {
        if (partitionLimit <= 0) return false;
        if ((int)points.size() <= partitionLimit) return false;
        if (level == targetLevel && targetLevel != -1) return false;
        return true;
    };



    if (checkPosition) {
        if ( !inBox(pos, center, size) ) { // not in node
            if (!parent) createParent();
            return parent->add(pos, dat, targetLevel, true, partitionLimit); // go a level up
        }
    }

    if (!reachedTargetLevel()) {
        int o = getOctant(pos);
        if (!children[o]) createChild(o);
        return children[o]->add(pos, dat, targetLevel, false, partitionLimit);
    }

    if (reachedPartitionLimit()) {
        while (size <= resolution) resolution *= 0.5;
        for (unsigned int i=0; i<points.size(); i++) {
            add(points[i], data[i], targetLevel, false, partitionLimit);
        }
        data.clear();
        points.clear();
        return add(pos, dat, targetLevel, false, partitionLimit);
    }

    data.push_back(dat);
    points.push_back(pos);
    return this;
}

void QuadtreeNode::set(QuadtreeNode* node, Vec3d p, void* d) { node->data.clear(); node->points.clear(); node->data.push_back(d); node->points.push_back(p); }

vector<QuadtreeNode*> QuadtreeNode::getAncestry() {
    vector<QuadtreeNode*> res;
    auto p = parent;
    while (p) { res.push_back(p); p = p->parent; }
    return res;
}

vector<QuadtreeNode*> QuadtreeNode::getChildren() {
    return vector<QuadtreeNode*>(children, children+8);
}

bool QuadtreeNode::isLeaf() {
    //return points.size() > 0;
    if ( resolution < size ) return false;
    for (int i=0; i<8; i++) if (children[i]) return false;
    return true;
}

vector<QuadtreeNode*> QuadtreeNode::getPathTo(Vec3d p) {
    vector<QuadtreeNode*> res;
    auto o = get(p);
    if (!o) return res;

    res.push_back(o);
    while (o->parent) {
        o = o->parent;
        res.push_back(o);
    }
    std::reverse(res.begin(), res.end());
    return res;
}

void gatherSubtree(QuadtreeNode* o, vector<QuadtreeNode*>& res, bool leafs) {
    for (auto c : o->getChildren()) {
        if (c) {
            if (leafs) {
                if (c->isLeaf()) res.push_back(c);
            } else res.push_back(c);
            gatherSubtree(c, res, leafs);
        }
    }
}

vector<QuadtreeNode*> QuadtreeNode::getSubtree() {
    vector<QuadtreeNode*> res;
    res.push_back(this);
    gatherSubtree(this, res, false);
    return res;
}

vector<QuadtreeNode*> QuadtreeNode::getLeafs() {
    vector<QuadtreeNode*> res;
    if (isLeaf()) res.push_back(this);
    gatherSubtree(this, res, true);
    return res;
}

Vec3d QuadtreeNode::getLocalCenter() {
    if (parent) return center - parent->center;
    else return center;
}

int QuadtreeNode::dataSize() { return data.size(); }

QuadtreeNode* QuadtreeNode::getParent() { return parent; }
QuadtreeNode* QuadtreeNode::getRoot() { auto o = this; while(o->parent) o = o->parent; return o; }

void QuadtreeNode::findInSphere(Vec3d p, float r, int d, vector<void*>& res) { // TODO: optimize!!
    if (!sphere_box_intersect(p, center, r, size)) return;

    float r2 = r*r;
    for (unsigned int i=0; i<data.size(); i++) {
        if ((points[i]-p).squareLength() <= r2)
            res.push_back(data[i]);
    }

    if (level == d && d != -1) return;
    for (int i=0; i<8; i++) {
        if (children[i]) children[i]->findInSphere(p, r, d, res);
    }
}

void QuadtreeNode::findPointsInSphere(Vec3d p, float r, int d, vector<Vec3d>& res, bool getAll) { // TODO: optimize!!
    if (!sphere_box_intersect(p, center, r, size)) return;

    float r2 = r*r;
    for (unsigned int i=0; i<data.size(); i++) {
        if ((points[i]-p).squareLength() <= r2) {
            res.push_back(points[i]);
            if (!getAll) return;
        }
    }

    if (level == d && d != -1) return;
    for (int i=0; i<8; i++) {
        if (children[i]) {
            children[i]->findPointsInSphere(p, r, d, res, getAll);
            if (!getAll && res.size() > 0) return;
        }
    }
}

void QuadtreeNode::findInBox(const Boundingbox& b, int d, vector<void*>& res) { // TODO: optimize!!
    if (!box_box_intersect(b.min(), b.max(), center, size)) return;

    for (unsigned int i=0; i<data.size(); i++) {
        if (b.isInside( points[i] )) res.push_back(data[i]);
    }

    if (level == d && d != -1) return;
    for (int i=0; i<8; i++) {
        if (children[i]) children[i]->findInBox(b, d, res);
    }
}

void QuadtreeNode::print(int indent) {
    cout << toString(indent) << flush;
    for (int i=0; i<8; i++) {
        if (children[i] != 0) children[i]->print(indent+1);
    }
}

vector<void*> QuadtreeNode::getAllData() {
    vector<void*> res;
    for (auto c : getSubtree()) {
        auto d = c->getData();
        res.insert(res.end(), d.begin(), d.end());
    }
    return res;
}


Quadtree::Quadtree(float res, float s, string n) : Partitiontree(res, s, n) {}
Quadtree::~Quadtree() { if (root) delete root; }

QuadtreePtr Quadtree::create(float resolution, float size, string n) {
    auto o = QuadtreePtr( new Quadtree(resolution, size, n) );
    o->clear();
    return o;
}

QuadtreePtr Quadtree::ptr() { return static_pointer_cast<Quadtree>(shared_from_this()); }

float Quadtree::getSize() { return root->getSize(); }
void Quadtree::setResolution(float res) { resolution = res; root->setResolution(res); }
void Quadtree::clear() { if (root) delete root; root = new QuadtreeNode(ptr(), resolution, firstSize, 0); }

QuadtreeNode* Quadtree::get(Vec3d p, bool checkPosition) { return root->get(p, checkPosition); }

vector<QuadtreeNode*> Quadtree::getAllLeafs() { return root->getRoot()->getLeafs(); }

QuadtreeNode* Quadtree::add(Vec3d p, void* data, int targetLevel, bool checkPosition, int partitionLimit) {
    return getRoot()->add(p, data, targetLevel, checkPosition, partitionLimit);
}

void Quadtree::addBox(const Boundingbox& b, void* d, int targetLevel, bool checkPosition) {
    const Vec3d min = b.min();
    const Vec3d max = b.max();
    add(min, d, targetLevel, checkPosition);
    add(Vec3d(max[0],min[1],min[2]), d, targetLevel, checkPosition);
    add(Vec3d(max[0],min[1],max[2]), d, targetLevel, checkPosition);
    add(Vec3d(min[0],min[1],max[2]), d, targetLevel, checkPosition);
    add(max, d, targetLevel, checkPosition);
    add(Vec3d(max[0],max[1],min[2]), d, targetLevel, checkPosition);
    add(Vec3d(min[0],max[1],min[2]), d, targetLevel, checkPosition);
    add(Vec3d(min[0],max[1],max[2]), d, targetLevel, checkPosition);
}

QuadtreeNode* Quadtree::getRoot() { return root; }
void Quadtree::updateRoot() { while (auto p = root->getParent()) root = p; }

vector<void*> Quadtree::getAllData() { return getRoot()->getAllData(); }

vector<void*> Quadtree::radiusSearch(Vec3d p, float r, int d) {
    vector<void*> res;
    getRoot()->findInSphere(p, r, d, res);
    return res;
}
vector<Vec3d> Quadtree::radiusPointSearch(Vec3d p, float r, int d, bool getAll) {
    vector<Vec3d> res;
    getRoot()->findPointsInSphere(p, r, d, res, getAll);
    return res;
}

vector<void*> Quadtree::boxSearch(const Boundingbox& b, int d) {
    vector<void*> res;
    getRoot()->findInBox(b, d, res);
    return res;
}

void Quadtree::test() {
    int Nv = 100000;
    float sMax = 4;
    Vec3d p(1,2,3);
    float r = 0.1;
    resolution = 0.0001;

    clear();
    srand(time(0));

    vector<Vec3d> Vec3fs;
    vector<void*> data;
    for (int i=0; i<Nv; i++) { // create random Vec3fs
        float x = rand()*sMax/RAND_MAX;
        float y = rand()*sMax/RAND_MAX;
        float z = rand()*sMax/RAND_MAX;
        auto d = (void*)new int(i);
        auto p = Vec3d(x,y,z);
        Vec3fs.push_back(p);
        data.push_back(d);
        add(p,d);
    }

    //getRoot()->print();

    int t0,t1,t2;
    vector<void*> radSearchRes_brute;

    t0=clock();
    vector<void*> radSearchRes_tree = radiusSearch(p, r);
    t1=clock();
    for (int i=0; i<Nv; i++) { // radius search brute forced
        auto p2 = Vec3fs[i];
        if (p2.dist(p) <= r) radSearchRes_brute.push_back( data[i] );
    }
    t2=clock();

    cout << "\ntest took " << t1-t0 << " octree range search && " << t2-t1 << " brute force\n";

    // validate results

    if (radSearchRes_brute.size() != radSearchRes_tree.size()) {
        cout << "\nQuadtreeNode test failed: result vector has wrong length " << radSearchRes_brute.size() << " " << radSearchRes_tree.size() << " !";
        return;
    }

    std::sort(radSearchRes_tree.begin(), radSearchRes_tree.end());
    std::sort(radSearchRes_brute.begin(), radSearchRes_brute.end());

    for (unsigned int i=0; i<radSearchRes_brute.size(); i++) {
        if (radSearchRes_tree[i] != radSearchRes_brute[i]) {
            cout << "\nQuadtreeNode test failed: mismatching test data!" << radSearchRes_tree[i] << "  " << radSearchRes_brute[i];
            return;
        }
    }

    cout << "\nQuadtreeNode test passed with " << radSearchRes_tree.size() << " found Vec3fs!\n";
}

VRGeometryPtr Quadtree::getVisualization(bool onlyLeafes) {
    VRGeoData data;
    vector<QuadtreeNode*> nodes;
    if (!onlyLeafes) {
        nodes = root->getRoot()->getSubtree();
        nodes.push_back(root);
    } else nodes = getAllLeafs();

    for (auto c : nodes) {
        Pnt3d p = c->getCenter();
        float s = c->getSize()*0.499;
        int ruf = data.pushVert(p + Vec3d( 1, 1, 1)*s);
        int luf = data.pushVert(p + Vec3d(-1, 1, 1)*s);
        int rub = data.pushVert(p + Vec3d( 1, 1,-1)*s);
        int lub = data.pushVert(p + Vec3d(-1, 1,-1)*s);
        int rdf = data.pushVert(p + Vec3d( 1,-1, 1)*s);
        int ldf = data.pushVert(p + Vec3d(-1,-1, 1)*s);
        int rdb = data.pushVert(p + Vec3d( 1,-1,-1)*s);
        int ldb = data.pushVert(p + Vec3d(-1,-1,-1)*s);
        data.pushQuad(ruf, luf, lub, rub); // side up
        data.pushQuad(rdf, ldf, ldb, rdb); // side down
        data.pushQuad(luf, lub, ldb, ldf); // side left
        data.pushQuad(ruf, rub, rdb, rdf); // side right
        data.pushQuad(rub, lub, ldb, rdb); // side behind
        data.pushQuad(ruf, luf, ldf, rdf); // side front
    }

    auto g = data.asGeometry("octree");
    auto m = VRMaterial::create("octree");
    m->setLit(false);
    m->setWireFrame(true);
    g->setMaterial(m);
    return g;
}


