#include "Octree.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <time.h>

OSG_BEGIN_NAMESPACE

OcPoint::OcPoint(Vec3f p, void* d) : pos(p), data(d) {}

Octree::Octree(float res) : resolution(res) {}

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

bool checkRecursion(Octree* t, Vec3f p) {
    static Octree* rec_1 = 0;
    static Octree* rec_2 = 0;
    static Octree* rec_3 = 0;

    if (t == rec_2 && rec_1 == rec_3) {
        cout << "\nRecursion error! Octree allready visited..";
        cout << "\n Vec3f " << p;
        cout << "\n Trees " << t << " " << rec_1 << " " << rec_2 << " " << rec_3;
        //cout << "\n parent center: "; t->center.print();
        cout << flush;
        return false;
    }

    rec_3 = rec_2;
    rec_2 = rec_1;
    rec_1 = t;

    return true;
}

bool Octree::inBox(Vec3f p, Vec3f c, float size) {
    if (abs(2*p[0] - 2*c[0]) > size) return false;
    if (abs(2*p[1] - 2*c[1]) > size) return false;
    if (abs(2*p[2] - 2*c[2]) > size) return false;
    return true;
}

void Octree::add(Vec3f p, void* d, int maxjump) { add(OcPoint(p, d), maxjump); }

void Octree::add(OcPoint p, int maxjump) {
    bool rOk = checkRecursion(this, p.pos);
    if (!rOk) {
        cout << "\n Center " << center;
        cout << "\n Size " << size;
        cout << "\n Data size " << data.size();
        cout << "\n In Box: " << inBox(p.pos, center, size) << endl;
        cout << "\n P Center " << parent->center;
        cout << "\n P Size " << parent->size;
        cout << "\n In Box: " << inBox(p.pos, parent->center, parent->size) << endl;
        cout << "\n P Vec3f Octant " <<  parent->getOctant(p.pos);
        cout << "\n P child at Vec3f Octant " <<  parent->children[ parent->getOctant(p.pos) ];
        cout << "\n this " << this;
        return;
    }

    //cout << "\nAdd "; p.print();
    Vec3f rp = p.pos - center;

    if ( !inBox(p.pos, center, size) ) {
        if (parent == 0) {
            float s2 = size*0.5;
            parent = new Octree(resolution);
            //Vec3f c = center.add( Vec3f(copysign(s2,rp[0]), copysign(s2,rp[1]), copysign(s2,rp[2])) );
            Vec3f c = center + lvljumpCenter(s2, rp);
            parent->center = c;
            float s = 2*size;
            parent->size = s;

            int o = parent->getOctant(center);
            parent->children[o] = this;
        }
        parent->add(p);
        return;
    }

    if (size > resolution && maxjump != 0) {
        int o = getOctant(p.pos);
        if (children[o] == 0) {
            float s2 = size*0.25;
            children[o] = new Octree(resolution);
            float s = size*0.5;
            children[o]->size = s;
            //Vec3f c = center.add( Vec3f(copysign(s2,rp[0]), copysign(s2,rp[1]), copysign(s2,rp[2])) );
            Vec3f c = center + lvljumpCenter(s2, rp);

            children[o]->center = c;
            children[o]->parent = this;
        }
        //Vec3f c = children[o]->center;
        children[o]->add(p, maxjump-1);
        return;
    }

    checkRecursion(0,p.pos);
    data.push_back(p.data);
}

void Octree::set(Octree* node, void* data) { node->data.clear(); node->data.push_back(data); }

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

    for (unsigned int i=0; i<data.size(); i++) {
        res.push_back(data[i]);
    }

    for (int i=0; i<8; i++) {
        if (children[i]) children[i]->findInSphere(p, r, res);
    }
}

vector<void*> Octree::radiusSearch(Vec3f p, float r) {
    vector<void*> res;
    getRoot()->findInSphere(p, r, res);
    return res;
}

void Octree::print(int indent) {
    cout << "\nOc ";
    for (int i=0; i<indent; i++) cout << " " ;
    cout << "size: " << size << " center: " << center;
    if (data.size() > 0) cout << endl;
    for (unsigned int i=0; i<data.size(); i++) if(data[i]) cout << " " << *(int*)data[i] ;
    cout << flush;

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

    vector<OcPoint> Vec3fs;
    for (int i=0; i<Nv; i++) { // create random Vec3fs
        float x = rand()*sMax/RAND_MAX;
        float y = rand()*sMax/RAND_MAX;
        float z = rand()*sMax/RAND_MAX;
        OcPoint p(Vec3f(x,y,z), (void*)new int(i));
        Vec3fs.push_back(p);
        add(p);
    }

    //getRoot()->print();

    int t0,t1,t2;
    vector<void*> radSearchRes_brute;

    t0=clock();
    vector<void*> radSearchRes_tree = radiusSearch(p, r);
    t1=clock();
    for (int i=0; i<Nv; i++) { // radius search brute forced
        auto p2 = Vec3fs[i];
        if (p2.pos.dist(p) < r) radSearchRes_brute.push_back( p2.data );
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
