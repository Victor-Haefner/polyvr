#include "Octree.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <time.h>
#include <sstream> //for std::stringstream
#include <string>  //for std::string

#include "core/utils/toString.h"

OSG_BEGIN_NAMESPACE

Octree::Octree(float res, float s) : resolution(res), size(s) {}
Octree::~Octree() {}

OctreePtr Octree::create(float resolution, float size) { return OctreePtr( new Octree(resolution, size) ); }

int Octree::getOctant(Vec3f p) {
    Vec3f rp = p - center;

    int o = 0;
    if (rp[0] < 0) o+=1;
    if (rp[1] < 0) o+=2;
    if (rp[2] < 0) o+=4;
    return o;
}

Vec3f lvljumpCenter(float s2, Vec3f rp) {
    Vec3f c(s2,s2,s2);
    if (rp[0] < 0) c[0]-=s2*2;
    if (rp[1] < 0) c[1]-=s2*2;
    if (rp[2] < 0) c[2]-=s2*2;
    return c;
}

bool Octree::inBox(Vec3f p, Vec3f c, float size) {
    if (abs(2*p[0] - 2*c[0]) > size) return false;
    if (abs(2*p[1] - 2*c[1]) > size) return false;
    if (abs(2*p[2] - 2*c[2]) > size) return false;
    return true;
}

void Octree::addBox(const boundingbox& b, void* d, int targetLevel, bool checkPosition) {
    const Vec3f min = b.min();
    const Vec3f max = b.max();
    add(min, d, targetLevel, 0, checkPosition);
    add(Vec3f(max[0],min[1],min[2]), d, targetLevel, 0, checkPosition);
    add(Vec3f(max[0],min[1],max[2]), d, targetLevel, 0, checkPosition);
    add(Vec3f(min[0],min[1],max[2]), d, targetLevel, 0, checkPosition);
    add(max, d, targetLevel, 0, checkPosition);
    add(Vec3f(max[0],max[1],min[2]), d, targetLevel, 0, checkPosition);
    add(Vec3f(min[0],max[1],min[2]), d, targetLevel, 0, checkPosition);
    add(Vec3f(min[0],max[1],max[2]), d, targetLevel, 0, checkPosition);
}

Octree* Octree::add(Vec3f p, void* d, int targetLevel, int currentLevel, bool checkPosition) {
    Vec3f rp = p - center;

    if ( !inBox(p, center, size) && checkPosition ) { // not in node
        if (parent == 0) { // no parent, create it
            parent = new Octree(resolution, 2*size);
            Vec3f c = center + lvljumpCenter(size*0.5, rp);
            parent->center = c;
            int o = parent->getOctant(center);
            parent->children[o] = this;
        }
        return parent->add(p, d, targetLevel, currentLevel+1, true); // go a level up
    }

    if (size > resolution && (currentLevel != targetLevel || targetLevel == -1)) {
        int o = getOctant(p);
        if (children[o] == 0) {
            children[o] = new Octree(resolution, size*0.5);
            Vec3f c = center + lvljumpCenter(size*0.25, rp);
            children[o]->center = c;
            children[o]->parent = this;
        }
        return children[o]->add(p, d, targetLevel, currentLevel-1, false);
    }

    data.push_back(d);
    points.push_back(p);
    return this;
}

void Octree::set(Octree* node, Vec3f p, void* d) { node->data.clear(); node->points.clear(); node->data.push_back(d); node->points.push_back(p); }

Octree* Octree::get(Vec3f p) {
    if ( !inBox(p, center, size) ) {
        return parent == 0 ? 0 : parent->get(p);
    }

    if (size > resolution) {
        int o = getOctant(p);
        if (children[o] == 0) return this;
        return children[o]->get(p);
    }

    return this;
}

vector<Octree*> Octree::getPathTo(Vec3f p) {
    vector<Octree*> res;
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

Vec3f Octree::getCenter() { return center; }
Vec3f Octree::getLocalCenter() { return parent ? center - parent->center : center; }

void Octree::destroy(Octree* guard) {
    for (int i=0; i<8; i++) {
        if (children[i] != 0) children[i]->destroy(guard);
        children[i] = 0;
    }

    if (guard != this) delete this;
    else {
        data.clear();
        parent = 0;
    }
}

void Octree::clear() { getRoot()->destroy(this); }

Octree* Octree::getRoot() {
    Octree* o = this;
    while (o->parent != 0) o = o->parent;
    return o;
}

Octree* Octree::getParent() { return parent; }

float Octree::getSize() { return size; }

// sphere center, box center, sphere radius, box size
bool sphere_box_intersect(Vec3f Ps, Vec3f Pb, float Rs, float Sb)  {
    float r2 = Rs * Rs;
    Vec3f diag(Sb*0.5, Sb*0.5, Sb*0.5);
    Vec3f Bmin = Pb - diag;
    Vec3f Bmax = Pb + diag;
    float dmin = 0;
    if( Ps[0] < Bmin[0] ) dmin += ( Ps[0] - Bmin[0] )*( Ps[0] - Bmin[0] );
    else if( Ps[0] > Bmax[0] ) dmin += ( Ps[0] - Bmax[0] )*( Ps[0] - Bmax[0] );
    if( Ps[1] < Bmin[1] ) dmin += ( Ps[1] - Bmin[1] )*( Ps[1] - Bmin[1] );
    else if( Ps[1] > Bmax[1] ) dmin += ( Ps[1] - Bmax[1] )*( Ps[1] - Bmax[1] );
    if( Ps[2] < Bmin[2] ) dmin += ( Ps[2] - Bmin[2] )*( Ps[2] - Bmin[2] );
    else if( Ps[2] > Bmax[2] ) dmin += ( Ps[2] - Bmax[2] )*( Ps[2] - Bmax[2] );
    return dmin <= r2;
}

void Octree::findInSphere(Vec3f p, float r, vector<void*>& res) { // TODO: optimize!!
    if (!sphere_box_intersect(p, center, r, size)) return;

    float r2 = r*r;
    for (unsigned int i=0; i<data.size(); i++) {
        if ((points[i]-p).squareLength() <= r2)
            res.push_back(data[i]);
    }

    for (int i=0; i<8; i++) {
        if (children[i]) children[i]->findInSphere(p, r, res);
    }
}

// box min, box max, octree box center, octree box size
bool box_box_intersect(Vec3f min, Vec3f max, Vec3f Bpos, float Sb)  {
    Vec3f Bdiag(Sb, Sb, Sb);
    Vec3f Bmin = Bpos - Bdiag*0.5;
    Vec3f Bmax = Bpos + Bdiag*0.5;

    Vec3f Apos = (max + min)*0.5;
    Vec3f Adiag = max-min;

    Vec3f diff = (Apos-Bpos)*2;
    Vec3f ABdiag = Adiag+Bdiag;
    return (abs(diff[0]) <= ABdiag[0]) && (abs(diff[1]) <= ABdiag[1]) && (abs(diff[2]) <= ABdiag[2]);
}

void Octree::findInBox(const boundingbox& b, vector<void*>& res) { // TODO: optimize!!
    if (!box_box_intersect(b.min(), b.max(), center, size)) return;

    for (unsigned int i=0; i<data.size(); i++) {
        if (b.isInside( points[i] )) res.push_back(data[i]);
    }

    for (int i=0; i<8; i++) {
        if (children[i]) children[i]->findInBox(b, res);
    }
}

vector<void*> Octree::radiusSearch(Vec3f p, float r) {
    vector<void*> res;
    getRoot()->findInSphere(p, r, res);
    return res;
}

vector<void*> Octree::boxSearch(const boundingbox& b) {
    vector<void*> res;
    getRoot()->findInBox(b, res);
    return res;
}

string Octree::toString(int indent) {
    auto pToStr = [](void* p) {
        const void * address = static_cast<const void*>(p);
        std::stringstream ss;
        ss << address;
        return ss.str();
    };

    string res = "\nOc ";
    for (int i=0; i<indent; i++) res += " ";
    res += "size: " + ::toString(size) + " center: " + ::toString(center);
    if (data.size() > 0) res += "\n";
    for (unsigned int i=0; i<data.size(); i++) if(data[i]) res += " " + pToStr(data[i]);
    return res;
}

void Octree::print(int indent) {
    cout << toString(indent) << flush;
    for (int i=0; i<8; i++) {
        if (children[i] != 0) children[i]->print(indent+1);
    }
}

void Octree::test() {
    int Nv = 100000;
    float sMax = 4;
    Vec3f p(1,2,3);
    float r = 0.1;
    resolution = 0.0001;

    clear();
    srand(time(0));

    vector<Vec3f> Vec3fs;
    vector<void*> data;
    for (int i=0; i<Nv; i++) { // create random Vec3fs
        float x = rand()*sMax/RAND_MAX;
        float y = rand()*sMax/RAND_MAX;
        float z = rand()*sMax/RAND_MAX;
        auto d = (void*)new int(i);
        auto p = Vec3f(x,y,z);
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
        cout << "\nOctree test failed: result vector has wrong length " << radSearchRes_brute.size() << " " << radSearchRes_tree.size() << " !";
        return;
    }

    std::sort(radSearchRes_tree.begin(), radSearchRes_tree.end());
    std::sort(radSearchRes_brute.begin(), radSearchRes_brute.end());

    for (unsigned int i=0; i<radSearchRes_brute.size(); i++) {
        if (radSearchRes_tree[i] != radSearchRes_brute[i]) {
            cout << "\nOctree test failed: mismatching test data!" << radSearchRes_tree[i] << "  " << radSearchRes_brute[i];
            return;
        }
    }

    cout << "\nOctree test passed with " << radSearchRes_tree.size() << " found Vec3fs!\n";
}

vector<void*> Octree::getData() { return data; }

OSG_END_NAMESPACE
