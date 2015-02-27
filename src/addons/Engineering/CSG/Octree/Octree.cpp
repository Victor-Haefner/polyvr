#include "Octree.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <time.h>

OSG_BEGIN_NAMESPACE

OcPoint::OcPoint(float x, float y, float z) {
    this->x = x;
    this->y = y;
    this->z = z;
    data=0;
}

float OcPoint::length() {
    return sqrt(x*x+y*y+z*z);
}

float OcPoint::dist(OcPoint p) {
    p = sub(p);
    return p.length();
}

OcPoint OcPoint::mult(float a) {
    return OcPoint(x*a, y*a, z*a);
}

OcPoint OcPoint::add(OcPoint p) {
    return OcPoint(x + p.x, y + p.y, z + p.z);
}

OcPoint OcPoint::sub(OcPoint p) {
    return OcPoint(x - p.x, y - p.y, z - p.z);
}

void OcPoint::print() {
    cout << "( " << x << ", "  << y << ", " << z << " )";
}

bool OcPoint::inBox(OcPoint c, float size) {
    if (abs(2*x - 2*c.x) > size) return false;
    if (abs(2*y - 2*c.y) > size) return false;
    if (abs(2*z - 2*c.z) > size) return false;
    return true;
}


Octree::Octree(float resolution) {
    this->resolution = resolution;
    this->size = 10;
    for (int i=0; i<8; i++) children[i] = 0;
    parent = 0;
}

int Octree::getOctant(OcPoint p) {
    OcPoint rp = p.sub(center);

    int o = 0;
    if (rp.x < 0) o+=1;
    if (rp.y < 0) o+=2;
    if (rp.z < 0) o+=4;
    return o;
}

OcPoint lvljumpCenter(float s2, OcPoint rp) {
    OcPoint c(s2,s2,s2);
    if (rp.x < 0) c.x-=s2*2;
    if (rp.y < 0) c.y-=s2*2;
    if (rp.z < 0) c.z-=s2*2;
    return c;
}

bool checkRecursion(Octree* t, OcPoint p) {
    static Octree* rec_1 = 0;
    static Octree* rec_2 = 0;
    static Octree* rec_3 = 0;


    if (t == rec_2 && rec_1 == rec_3) {
        cout << "\nRecursion error! Octree allready visited..";
        cout << "\n OcPoint "; p.print();
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

void Octree::add(OcPoint p, void* data, int maxjump) {
    bool rOk = checkRecursion(this, p);
    if (!rOk) {
        cout << "\n Center "; center.print();
        cout << "\n Size " << size;
        cout << "\n Data size " << this->data.size();
        cout << "\n In Box: " << p.inBox(center, size) << endl;
        cout << "\n P Center "; parent->center.print();
        cout << "\n P Size " << parent->size;
        cout << "\n In Box: " << p.inBox(parent->center, parent->size) << endl;
        cout << "\n P OcPoint Octant " <<  parent->getOctant(p);
        cout << "\n P child at OcPoint Octant " <<  parent->children[ parent->getOctant(p) ];
        cout << "\n this " << this;
        return;
    }

    //cout << "\nAdd "; p.print();
    OcPoint rp = p.sub(center);

    if ( ! p.inBox(center, size) ) {
        if (parent == 0) {
            float s2 = size*0.5;
            parent = new Octree(resolution);
            //OcPoint c = center.add( OcPoint(copysign(s2,rp.x), copysign(s2,rp.y), copysign(s2,rp.z)) );
            OcPoint c = center.add( lvljumpCenter(s2, rp) );
            parent->center = c;
            float s = 2*size;
            parent->size = s;

            int o = parent->getOctant(center);
            parent->children[o] = this;
        }
        parent->add(p, data);
        return;
    }

    if (size > resolution && maxjump != 0) {
        int o = getOctant(p);
        if (children[o] == 0) {
            float s2 = size*0.25;
            children[o] = new Octree(resolution);
            float s = size*0.5;
            children[o]->size = s;
            //OcPoint c = center.add( OcPoint(copysign(s2,rp.x), copysign(s2,rp.y), copysign(s2,rp.z)) );
            OcPoint c = center.add( lvljumpCenter(s2, rp) );

            children[o]->center = c;
            children[o]->parent = this;
        }
        //OcPoint c = children[o]->center;
        children[o]->add(p, data, maxjump-1);
        return;
    }

    checkRecursion(0,p);
    this->data.push_back(data);
}

void Octree::add(float x, float y, float z, void* data, int maxjump) {
    add(OcPoint(x,y,z), data, maxjump);
}

void Octree::set(Octree* node, void* data) { node->data.clear(); node->data.push_back(data); }

Octree* Octree::get(float x, float y, float z) {
    OcPoint p = OcPoint(x,y,z);
    if ( ! p.inBox(center, size) ) {
        return parent == 0 ? 0 : parent->get(x,y,z);
    }

    if (size > resolution) {
        int o = getOctant(p);
        if (children[o] == 0) return this;
        return children[o]->get(x,y,z);
    }

    return this;
}

void Octree::destroy(Octree* guard) {
    for (int i=0; i<8; i++) {
        if (children[i] != 0) children[i]->destroy(guard);
        children[i] = 0;
    }

    if (guard != this) delete this;
}

void Octree::clear() {
    getRoot()->destroy(this);
}

Octree* Octree::getRoot() {
    Octree* o = this;
    while (o->parent != 0) o = o->parent;
    return o;
}

// sphere center, box center, sphere radius, box size
bool sphere_box_intersect(OcPoint Ps, OcPoint Pb, float Rs, float Sb)  {
    float r2 = Rs * Rs;
    OcPoint diag(Sb*0.5, Sb*0.5, Sb*0.5);
    OcPoint Bmin = Pb.sub(diag);
    OcPoint Bmax = Pb.add(diag);
    float dmin = 0;
    if( Ps.x < Bmin.x ) dmin += ( Ps.x - Bmin.x )*( Ps.x - Bmin.x );
    else if( Ps.x > Bmax.x ) dmin += ( Ps.x - Bmax.x )*( Ps.x - Bmax.x );
    if( Ps.y < Bmin.y ) dmin += ( Ps.y - Bmin.y )*( Ps.y - Bmin.y );
    else if( Ps.y > Bmax.y ) dmin += ( Ps.y - Bmax.y )*( Ps.y - Bmax.y );
    if( Ps.z < Bmin.z ) dmin += ( Ps.z - Bmin.z )*( Ps.z - Bmin.z );
    else if( Ps.z > Bmax.z ) dmin += ( Ps.z - Bmax.z )*( Ps.z - Bmax.z );
    return dmin <= r2;
}

void Octree::findInSphere(OcPoint p, float r, vector<void*>& res) { // TODO: optimize!!
    if (!sphere_box_intersect(p, center, r, size)) return;

    for (unsigned int i=0; i<data.size(); i++) {
        res.push_back(data[i]);
    }

    for (int i=0; i<8; i++) {
        if (children[i]) children[i]->findInSphere(p, r, res);
    }
}

vector<void*> Octree::radiusSearch(OcPoint p, float r) {
    vector<void*> res;

    getRoot()->findInSphere(p, r, res);

    return res;
}

vector<void*> Octree::radiusSearch(float x, float y, float z, float r) {
    return radiusSearch(OcPoint(x,y,z), r);
}

void Octree::print(int indent) {
    cout << "\nOc ";
    for (int i=0; i<indent; i++) cout << " " ;
    cout << "size: " << size << " center: "; center.print();
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
    OcPoint p(1,2,3);
    float r = 0.1;
    resolution = 0.0001;

    clear();
    srand(time(0));

    vector<OcPoint> OcPoints;
    for (int i=0; i<Nv; i++) { // create random OcPoints
        float x = rand()*sMax/RAND_MAX;
        float y = rand()*sMax/RAND_MAX;
        float z = rand()*sMax/RAND_MAX;
        OcPoint _p(x,y,z);
        _p.data = (void*)new int(i);
        OcPoints.push_back( _p );
        add( OcPoints[i], OcPoints[i].data );
    }

    //getRoot()->print();

    int t0,t1,t2;
    vector<void*> radSearchRes_brute;

    t0=clock();
    vector<void*> radSearchRes_tree = radiusSearch(p, r);
    t1=clock();
    for (int i=0; i<Nv; i++) { // radius search brute forced
        OcPoint p2 = OcPoints[i];
        if (p2.dist(p) < r) radSearchRes_brute.push_back( p2.data );
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

    cout << "\nOctree test passed with " << radSearchRes_tree.size() << " found OcPoints!\n";
}

vector<void*> Octree::getData() {
	return data;
}

OSG_END_NAMESPACE
