#ifndef OCTREET_H_INCLUDED
#define OCTREET_H_INCLUDED

#include "Octree.h"
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

#include "PartitiontreeT.h"

template<class T>
OctreeNode<T>* OSG::OctreeNode<T>::get(Vec3d p, bool checkPosition) {
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

template<class T>
OctreeNode<T>* OctreeNode<T>::extend(Vec3d pos, int targetLevel, bool checkPosition) {
    Vec3d rpos = pos - center;

    auto createParent = [&]() {
        parent = new OctreeNode(tree.lock(), resolution, 2*size, level+1);
        parent->center = center + lvljumpCenter(size*0.5, rpos);
        int o = parent->getOctant(center);
        parent->children[o] = this;
        tree.lock()->updateRoot();
    };

    auto createChild = [&](int octant) {
        children[octant] = new OctreeNode(tree.lock(), resolution, size*0.5, level-1);
        Vec3d c = center + lvljumpCenter(size*0.25, rpos);
        children[octant]->center = c;
        children[octant]->parent = this;
    };

    auto reachedTargetLevel = [&]() {
        if (size <= resolution) return true;
        if (level == targetLevel && targetLevel != -1) return true;
        return false;
    };

    if (checkPosition) {
        if ( !inBox(pos, center, size) ) { // not in node
            if (size > 1e12) return 0;
            if (!parent) createParent();
            return parent->extend(pos, targetLevel, true); // go a level up
        }
    }

    if (!reachedTargetLevel()) {
        int o = getOctant(pos);
        if (!children[o]) createChild(o);
        return children[o]->extend(pos, targetLevel, false);
    }
    return this;
}

// partitionLimit sets a max amount of data points, tree is subdivided if necessary!

template<class T>
OctreeNode<T>* OctreeNode<T>::add(Vec3d pos, T dat, int targetLevel, bool checkPosition, int partitionLimit) {
    Vec3d rpos = pos - center;

    auto createParent = [&]() {
        parent = new OctreeNode(tree.lock(), resolution, 2*size, level+1);
        parent->center = center + lvljumpCenter(size*0.5, rpos);
        int o = parent->getOctant(center);
        parent->children[o] = this;
        tree.lock()->updateRoot();
    };

    auto createChild = [&](int octant) {
        children[octant] = new OctreeNode(tree.lock(), resolution, size*0.5, level-1);
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

    // TODO: there is a bug if adding millions of zeros..
    /*static int N = 0;
    N++;
    if (N == 100000) {
        cout << "octreenode add " << this << " " << pos << " rpos " << rpos << " size " << size << " N pnts " << points.size() << " " << reachedPartitionLimit() << " partitionLimit " << partitionLimit << endl;
        N = 0;
    }*/

    if (checkPosition) {
        if ( !inBox(pos, center, size) ) { // not in node
            //cout << "OctreeNode<T>::add, out of box " << size << " -> go up!" << endl;
            if (size > 1e12) {
                //cout << "OctreeNode<T>::add, out of box but size too big: " << size << " -> skip point " << pos << endl;
                return 0;
            }
            if (!parent) createParent();
            return parent->add(pos, dat, targetLevel, true, partitionLimit); // go a level up
        }
    }

    if (!reachedTargetLevel()) {
        int o = getOctant(pos);
        if (!children[o]) createChild(o);
        //cout << "OctreeNode<T>::add, not yet target level reached (" << targetLevel << ", " << pos << ", " << o << ", " << size <<  ") -> go down!" << endl;
        /*cout << "OctreeNode<T>::add, not yet target level reached (" << targetLevel << ", " << pos << ", " << o << ", " << size <<  ") -> go down! ";
        cout << DBL_MAX << ", " << bool(size > DBL_MAX/4) << endl;
        cout << endl;*/
        return children[o]->add(pos, dat, targetLevel, false, partitionLimit);
    }

    if (reachedPartitionLimit()) {
        //cout << "OctreeNode<T>::add, reachedPartitionLimit (" << partitionLimit << ") -> split node!" << endl;
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

template<class T>
int OctreeNode<T>::getOctant(Vec3d p) {
    Vec3d rp = p - center;

    int o = 0;
    if (rp[0] < 0) o+=1;
    if (rp[1] < 0) o+=2;
    if (rp[2] < 0) o+=4;
    return o;
}

template<class T>
Vec3d OctreeNode<T>::lvljumpCenter(float s2, Vec3d rp) {
    Vec3d c(s2,s2,s2);
    if (rp[0] < 0) c[0]-=s2*2;
    if (rp[1] < 0) c[1]-=s2*2;
    if (rp[2] < 0) c[2]-=s2*2;
    return c;
}

template<class T>
bool OctreeNode<T>::inBox(Vec3d p, Vec3d c, float size) {
    if (abs(2*p[0] - 2*c[0]) > size) return false;
    if (abs(2*p[1] - 2*c[1]) > size) return false;
    if (abs(2*p[2] - 2*c[2]) > size) return false;
    return true;
}

template<class T>
void OctreeNode<T>::set(OctreeNode* node, Vec3d p, T d) { node->data.clear(); node->points.clear(); node->data.push_back(d); node->points.push_back(p); }

template<class T>
vector<OctreeNode<T>*> OctreeNode<T>::getAncestry() {
    vector<OctreeNode<T>*> res;
    auto p = parent;
    while (p) { res.push_back(p); p = p->parent; }
    return res;
}

template<class T>
vector<OctreeNode<T>*> OctreeNode<T>::getChildren() {
    return vector<OctreeNode<T>*>(children, children+8);
}

template<class T>
bool OctreeNode<T>::isLeaf() {
    //return points.size() > 0;
    if ( resolution < size ) return false;
    for (int i=0; i<8; i++) if (children[i]) return false;
    return true;
}

template<class T>
vector<OctreeNode<T>*> OctreeNode<T>::getPathTo(Vec3d p) {
    vector<OctreeNode<T>*> res;
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

template<class T>
void gatherSubtree(OctreeNode<T>* o, vector<OctreeNode<T>*>& res, bool leafs) {
    for (auto c : o->getChildren()) {
        if (c) {
            if (leafs) {
                if (c->isLeaf()) res.push_back(c);
            } else res.push_back(c);
            gatherSubtree(c, res, leafs);
        }
    }
}

template<class T>
vector<OctreeNode<T>*> OctreeNode<T>::getSubtree() {
    vector<OctreeNode<T>*> res;
    res.push_back(this);
    gatherSubtree(this, res, false);
    return res;
}

template<class T>
vector<OctreeNode<T>*> OctreeNode<T>::getLeafs() {
    vector<OctreeNode<T>*> res;
    if (isLeaf()) res.push_back(this);
    gatherSubtree(this, res, true);
    return res;
}

template<class T>
Vec3d OctreeNode<T>::getLocalCenter() {
    if (parent) return center - parent->center;
    else return center;
}

template<class T>
int OctreeNode<T>::dataSize() { return data.size(); }

template<class T>
OctreeNode<T>* OctreeNode<T>::getParent() { return parent; }
template<class T>
OctreeNode<T>* OctreeNode<T>::getRoot() { auto o = this; while(o->parent) o = o->parent; return o; }

template<class T>
void OctreeNode<T>::findInSphere(Vec3d p, float r, int d, vector<T>& res) { // TODO: optimize!!
    if (!this->sphere_box_intersect(p, center, r, size)) return;

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

template<class T>
void OctreeNode<T>::findPointsInSphere(Vec3d p, float r, int d, vector<Vec3d>& res, bool getAll) { // TODO: optimize!!
    if (!this->sphere_box_intersect(p, center, r, size)) return;

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

template<class T>
void OctreeNode<T>::findInBox(const Boundingbox& b, int d, vector<T>& res) { // TODO: optimize!!
    if (!this->box_box_intersect(b.min(), b.max(), center, size)) return;

    for (unsigned int i=0; i<data.size(); i++) {
        if (b.isInside( points[i] )) res.push_back(data[i]);
    }

    if (level == d && d != -1) return;
    for (int i=0; i<8; i++) {
        if (children[i]) children[i]->findInBox(b, d, res);
    }
}

template<class T>
void OctreeNode<T>::print(int indent) {
    cout << toString(indent) << flush;
    for (int i=0; i<8; i++) {
        if (children[i] != 0) children[i]->print(indent+1);
    }
}

template<class T>
vector<T> OctreeNode<T>::getAllData() {
    vector<T> res;
    for (auto c : getSubtree()) {
        auto d = c->getData();
        res.insert(res.end(), d.begin(), d.end());
    }
    return res;
}

template<class T>
size_t OctreeNode<T>::countNodes() {
    size_t n = 1;
    for (auto c : children) if (c) n += c->countNodes();
    return n;
}

template<class T>
int OctreeNode<T>::getDepth() {
    int depth = 0;
    for (auto& c : children) if (c) depth = max(depth, c->getDepth()) + 1;
    return depth;
}

template<class T>
OctreeNode<T>* OctreeNode<T>::getChild(int i) { if (i >= 0 && i < 8) return children[i]; return 0; }


template<class T>
shared_ptr<Octree<T>> Octree<T>::create(float resolution, float size, string n) {
    auto o = shared_ptr<Octree<T>>( new Octree<T>(resolution, size, n) );
    o->clear();
    return o;
}

template<class T>
shared_ptr<Octree<T>> Octree<T>::ptr() { return static_pointer_cast<Octree<T>>(shared_from_this()); }

template<class T>
int Octree<T>::getDepth() { return root->getDepth(); }

template<class T>
float Octree<T>::getSize() { return root->getSize(); }

template<class T>
void Octree<T>::setResolution(float res) { resolution = res; root->setResolution(res); }

template<class T>
void Octree<T>::clear() { if (root) delete root; root = new Octree::Node(ptr(), resolution, firstSize, 0); }

template<class T>
OctreeNode<T>* Octree<T>::get(Vec3d p, bool checkPosition) { return root->get(p, checkPosition); }

template<class T>
vector<OctreeNode<T>*> Octree<T>::getAllLeafs() { return root->getRoot()->getLeafs(); }

template<class T>
size_t Octree<T>::getNodesCount() { return root->getRoot()->countNodes(); }

template<class T>
OctreeNode<T>* Octree<T>::add(Vec3d p, T data, int targetLevel, bool checkPosition, int partitionLimit) {
    return getRoot()->add(p, data, targetLevel, checkPosition, partitionLimit);
}

template<class T>
OctreeNode<T>* Octree<T>::extend(Vec3d p, int targetLevel, bool checkPosition) {
    return getRoot()->extend(p, targetLevel, checkPosition);
}

template<class T>
void Octree<T>::addBox(const Boundingbox& b, T d, int targetLevel, bool checkPosition) {
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

template<class T>
OctreeNode<T>* Octree<T>::getRoot() { return root; }

template<class T>
void Octree<T>::updateRoot() { while (auto p = root->getParent()) root = p; }

template<class T>
double Octree<T>::getLeafSize() {
    double s = root->getSize();
    while ( resolution < s ) s *= 0.5;
    return s;
}

template<class T>
vector<T> Octree<T>::getAllData() { return getRoot()->getAllData(); }

template<class T>
vector<T> Octree<T>::radiusSearch(Vec3d p, float r, int d) {
    vector<T> res;
    getRoot()->findInSphere(p, r, d, res);
    return res;
}

template<class T>
vector<Vec3d> Octree<T>::radiusPointSearch(Vec3d p, float r, int d, bool getAll) {
    vector<Vec3d> res;
    getRoot()->findPointsInSphere(p, r, d, res, getAll);
    return res;
}

template<class T>
vector<T> Octree<T>::boxSearch(const Boundingbox& b, int d) {
    vector<T> res;
    getRoot()->findInBox(b, d, res);
    return res;
}

template<class T>
void Octree<T>::test() {
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
        cout << "\nOctreeNode test failed: result vector has wrong length " << radSearchRes_brute.size() << " " << radSearchRes_tree.size() << " !";
        return;
    }

    std::sort(radSearchRes_tree.begin(), radSearchRes_tree.end());
    std::sort(radSearchRes_brute.begin(), radSearchRes_brute.end());

    for (unsigned int i=0; i<radSearchRes_brute.size(); i++) {
        if (radSearchRes_tree[i] != radSearchRes_brute[i]) {
            cout << "\nOctreeNode test failed: mismatching test data!" << radSearchRes_tree[i] << "  " << radSearchRes_brute[i];
            return;
        }
    }

    cout << "\nOctreeNode test passed with " << radSearchRes_tree.size() << " found Vec3fs!\n";
}

template<class T>
VRGeometryPtr Octree<T>::getVisualization(bool onlyLeafes) {
    VRGeoData data;
    vector<Octree::Node*> nodes;
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


#endif // OCTREET_H_INCLUDED
