#include "CKOctree.h"
#include "core/math/Octree.h"

OSG_BEGIN_NAMESPACE
using namespace std;

CKOctree::element::element(CKOctree* tree, Vec3f p, Vec3i otp, int s) : pos(p), otpos(otp), size(s), _size(s / 2) {
    for (int i=0;i<8;i++) {
        children[i] = 0;
        octIsEmpty[i] = 0;
        float k = 0.2;
        if (i<6) vertexLight[i] = Vec4f(k,k,k,k);
    }

    static int count = 0;
    count++;
    ID = count;

    this->tree = tree;
    tree->elements[ID] = this;
}

CKOctree::element::~element() {
    tree->elements.erase(ID);
}

void CKOctree::element::add(element* e) {
    if (e == 0) return;

    Vec3i d = e->otpos - otpos;
    int o = getOctant(d);
    element* c = e;

    if (e->size*2 != size) {
        c = children[o];
        if (c == 0) {
            Vec3i otp = otpos + getOctantVec(o);
            Vec3f p = (Vec3f(otp) - Vec3f(1,1,1))*0.5 ;
            c = new element(tree, p, otp, size/2);
        }
    }

    c->parent = this;
    c->octant = o;
    children[o] = c;
    octIsEmpty[o] = 0;
    childN++;

    if (c != e) c->add(e);
}

int CKOctree::element::getOctant(Vec3i p) { return getOctant(Vec3f(p)); }
int CKOctree::element::getOctant(Vec3f p) {
    if(p[0] >= 0) {
        if(p[1] >= 0) {
            if(p[2] >= 0) return 0;
            else return 1;
        } else {
            if(p[2] >= 0) return 2;
            else return 3;
        }
    } else {
        if(p[1] >= 0) {
            if(p[2] >= 0) return 4;
            else return 5;
        } else {
            if(p[2] >= 0) return 6;
            else return 7;
        }
    }
}

Vec3i CKOctree::element::getOctantVec(int o) {
    Vec3i d;
    switch(o) {
        case 0:
            d = Vec3i( 1, 1, 1);
            break;
        case 1:
            d = Vec3i( 1, 1,-1);
            break;
        case 2:
            d = Vec3i( 1,-1, 1);
            break;
        case 3:
            d = Vec3i( 1,-1,-1);
            break;
        case 4:
            d = Vec3i(-1, 1, 1);
            break;
        case 5:
            d = Vec3i(-1, 1,-1);
            break;
        case 6:
            d = Vec3i(-1,-1, 1);
            break;
        case 7:
            d = Vec3i(-1,-1,-1);
            break;
    }
    d *= _size;
    return d;
}

bool CKOctree::element::inside(Vec3f f) {
    for (int i=0;i<3;i++) {
        if (abs(f[i] - pos[i]) > size*0.5) return false;
    }
    return true;
}

bool CKOctree::element::inside(Vec3i f) {
    for (int i=0;i<3;i++) {
        float d = abs(f[i] - otpos[i]);
        cout << "\ninside " << d << " s " << size << flush;
        if (d > size) return false;
    }
    return true;
}

void CKOctree::element::print(string indent) {
    cout << "\n" << indent << "Octree element " << ID << " size: " << size;
    cout << " at " << pos << "  :  " << otpos << " in octant " << octant << flush;
}

// -----------------------------------------------------------------------------------------------------------

int CKOctree::getMax(Vec3i i) {
    int r = abs(i[0]);
    if (abs(i[1]) > r) r = abs(i[1]);
    if (abs(i[2]) > r) r = abs(i[2]);
    return r;
}

void CKOctree::print(element* e, string indent) {
    if (e == 0) return;

    e->print(indent);

    for (int i=0;i<8;i++)
        print(e->children[i], indent + " ");
}

int CKOctree::signof(float f) {
    if (f >= 0) return 1;
    return -1;
}

CKOctree::CKOctree() {
	lightTree = new Octree(1);
}

CKOctree::element* CKOctree::add(Vec3i _p) {
    Vec3i p = _p*2 + Vec3i(1,1,1); // avoid half positions

    int s = getMax(p);
    int i = 1;
    while(s+1 > i) i *= 2;
    s = i;

    if (root == 0) root = new element(this, Vec3f(-0.5, -0.5, -0.5), Vec3i(), s);

    if (root->size < s) {
        element* tmp = root;
        root = new element(this, Vec3f(-0.5, -0.5, -0.5), Vec3i(), s);
        for (int i=0;i<8;i++) root->add(tmp->children[i]);
        delete tmp;
    }

    element* cube = new element(this, Vec3f(_p), p, 1);
    cube->leaf = true;
    root->add(cube);
    cube->updateLightning(lightTree->radiusSearch(Vec3f(_p), 10));
    N++;

    return cube;
}

CKOctree::light* CKOctree::addLight(Vec3f p) {
    auto l = new light(p);
    lightTree->add(p, l);
    return l;
}

void CKOctree::element::updateLightning(CKOctree::light* l) {
    Vec3f normals[6];
    normals[0]= Vec3f(1,0,0);
    normals[1]= Vec3f(0,1,0);
    normals[2]= Vec3f(0,0,1);
    normals[3]= Vec3f(-1,0,0);
    normals[4]= Vec3f(0,-1,0);
    normals[5]= Vec3f(0,0,-1);

    Vec3f tangents[6];
    tangents[0] = Vec3f( 0, 1, 0);
    tangents[1] = Vec3f( 0, 0, 1);
    tangents[2] = Vec3f( 1, 0, 0);
    tangents[3] = Vec3f( 0, 1, 0);
    tangents[4] = Vec3f( 0, 0, 1);
    tangents[5] = Vec3f( 1, 0, 0);

    Vec2f ric[4];
    ric[0] = Vec2f(1,1);
    ric[1] = Vec2f(-1,1);
    ric[2] = Vec2f(1,-1);
    ric[3] = Vec2f(-1,-1);

    Vec3f lp = l->pos;
    Vec3f dp = lp - pos;
    Vec3f n, p, t, x;
    for (int j=0;j<6;j++) {
        n = normals[j];
        t = tangents[j];
        x = t.cross(n);
        for (int k=0;k<4;k++) {
            p = dp + (n + ric[k][0]*t + ric[k][1]*x)*0.5;

            double a = 0.5 + 0.0*p.squareLength() + 1.0*p.length();
            p.normalize();

            float cos = -p*n;
            if (cos < 0) continue;

            float li = min(0.1+2.0*cos/a, 2.0);
            vertexLight[j][k] = max(li, vertexLight[j][k]);
        }
    }
}

void CKOctree::element::updateLightning(vector<void*> lights) {
    for (auto vl : lights) updateLightning((light*)vl);
}

void CKOctree::rem(element* e) {
    N--;

    element* p = e->parent;
    if (p != 0) {
        int o = e->octant;
        p->children[o] = 0;
        p->octIsEmpty[o] = true;
        p->childN--;
    }
    if (e == root) root = 0;

    delete e;

    if (p != 0)
        if (p->childN == 0) rem(p);
}

void CKOctree::addAround(element* e) {
    Vec3i p = e->otpos;
    int i,j,k;
    Vec3i p2;
    Vec3f pos;

    for (i=-1;i<2;i++) {
        for (j=-1;j<2;j++) {
            for (k=-1;k<2;k++) {
                if (i==0 && j==0 && k==0) continue; //ignore itself

                p2 = p + Vec3i(i,j,k);
                pos = e->pos + Vec3f(i,j,k);
                if ( isLeaf(pos) ) continue;
                if ( isEmpty(pos) ) continue;

                //solid rock, add cube
                add(Vec3i(pos));
            }
        }
    }
}


//check if there is a cube at pos
bool CKOctree::isLeaf(Vec3f p) {
    element* res = get(p);
    if (res == 0) return false;

    if (res->size == 1) return true;

    return false;
}

//check if space || solid at pos
bool CKOctree::isEmpty(Vec3f p) {
    element* res = get(p);
    if (res == 0) return false;

    int o = res->getOctant(p - res->pos);
    return res->octIsEmpty[o];
}

void CKOctree::setEmpty(Vec3i p) {
    element* e = get(Vec3f(p));
    if (e == 0) return;

    int o = e->getOctant(Vec3f(p) - e->pos);
    e->octIsEmpty[o] = true;
}

//get the smallest element at that position
CKOctree::element* CKOctree::get(Vec3f p, CKOctree::element* e) {
    if (e == 0) e = root;
    if (e == 0) return 0;

    if (e->childN == 0) return e;

    for (int i=0; i<8; i++) {
        element* c = e->children[i];
        if (c == 0) continue;
        if (c->inside(p)) return get(p,c);
    }

    return e;
}

//ray cast
CKOctree::element* CKOctree::get(Line ray, CKOctree::element* e, string indent) {
    if (e == 0) e = root;
    if (e == 0) return 0;

    Vec3f pos = Vec3f(ray.getPosition());
    hitPoint = pos;
    hitElement = e;
    hitNormal = pos - Vec3f(e->pos);
    for (int i=0;i<3;i++) {
        int j = (i+1)%3;
        int k = (i+2)%3;
        if (abs(hitNormal[i]) > abs(hitNormal[j]) &&
            abs(hitNormal[i]) > abs(hitNormal[k])) {
                hitNormal[i] *= 2;
                hitNormal[j]  = 0;
                hitNormal[k]  = 0;
                break;
        }
    }

    if (!e->inside(pos)) return 0;

    //found cube
    if (e->size == 1) return e;


    Vec3f dir = ray.getDirection();
    Vec3f epos = Vec3f(e->pos);
    Vec3f diff = pos - epos;

    int o_dir = e->getOctant(dir);
    int o_pos = e->getOctant(diff);
    element* c = e->children[o_pos];

    if (o_dir == o_pos && c == 0) {
        return 0; //cast in empty space
    }

    //check first in depht
    if (c != 0) {
        element* res = get(ray, c, indent + " ");
        if (res) return res;
    }

    //pos octant empty, but casting a ray into another octant!

    //get octants intersected by ray
    map<float, int> octants;
    map<float, int>::iterator itr;
    map<int, Vec3f> ipoints;
    for (int i=0; i<3;i++) {
        if (abs(dir[i]) < 1e-6) continue;

        float k = (epos[i] - pos[i])/dir[i]; //intersection with plane i
        if (k < 0) continue;

        Vec3f r = pos + k*dir; //intersection point

        Vec3f tmp = r - epos;
        tmp[i] = -signof(diff[i]); //move to other side of plane
        int o = e->getOctant(tmp); //get octant

        octants[k] = o;
        ipoints[o] = r;
    }

    //recursion
    for (itr = octants.begin(); itr != octants.end(); itr++) {
        if (itr->first < 0) continue;

        c = e->children[itr->second];
        if (c != 0) {
            ray.setValue(Pnt3f(ipoints[itr->second]), dir);
            element* res = get(ray, c, indent + " ");
            //hitPoint = ipoints[itr->second];
            if (res) return res;
        }
    }

    return 0;
}

vector<CKOctree::element*> CKOctree::getAround(Vec3f pos, float r) {
    //cout << "\nRequest Around " << pos << " in R " << r;
    vector<element*> elements;
    element* oe = 0;


    int R = (r+1);
    for (int i=-R; i<=R; i++) {
        for (int j=-R; j<=R; j++) {
            for (int k=-R; k<=R; k++) {
                Vec3f p = pos+Vec3f(i,j,k);
                element* e = get(p);
                if (e == 0) continue;
                if (e->size == 1 && e != oe) {
                    elements.push_back(e);
                    oe = e;
                    //cout << "\nAt " << p << " is " << e->size << " ptr " << e ;
                }
            }
        }
    }


    //cout << "\nFound " << elements.size() << " elements " << flush;
    return elements;
}

void CKOctree::traverse(VRFunction<element*>* cb) { traverse(root, cb); }
void CKOctree::traverse(element* e, VRFunction<element*>* cb) {
    if (e == 0) return;
    if (e->leaf) (*cb)(e);
    for (int i=0; i<8; i++) traverse(e->children[i], cb);
}

CKOctree::element* CKOctree::getRoot() { return root; }
CKOctree::element* CKOctree::getElement(int i) { if (elements.count(i)) return elements[i]; else return 0; }

Vec3f CKOctree::getHitPoint() { return hitPoint; }
Vec3f CKOctree::getHitNormal() { return hitNormal; }
CKOctree::element* CKOctree::getHitElement() { return hitElement; }

void CKOctree::print() {
    if (root == 0) cout << "\nTree is empty" << flush;
    print(root);
}

OSG_END_NAMESPACE
