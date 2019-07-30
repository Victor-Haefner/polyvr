#include "patch.h"

#include "core/utils/toString.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/material/VRMaterial.h"

#include <OpenSG/OSGFaceIterator.h>
#include <OpenSG/OSGTriangleIterator.h>

using namespace std;
using namespace OSG;

template<> string typeName(const OSG::Patch& t) { return "Patch"; }

Patch::Patch() {}
Patch::~Patch() {}
PatchPtr Patch::create() { return PatchPtr(new Patch()); }

template <int S>
Patch::bezVRPolygon<S>::bezVRPolygon() {
    initVRPolygon();
}

template <int S>
Patch::bezVRPolygon<S>::~bezVRPolygon() {}

template <int S>
void Patch::bezVRPolygon<S>::initVRPolygon() {
    p = vector<Vec3d>(S);
    n = vector<Vec3d>(S);
    tex = vector<Vec2d>(S);

    for (int i=0;i<S;i++) {
        for (int j=0;j<3;j++){
            this->p[i][j]=0;
            this->n[i][j]=0;
            if (j<2) this->tex[i][j]=0;
        }
    }
}


Vec3d Patch::projectInPlane(Vec3d v, Vec3d n, bool keep_length) {
    n.normalize();
    float l;
    if (keep_length) l = v.length();
    float k = v.dot(n);
    v -= n*k;
    if (keep_length) v = v*(l/v.length());
    return v;
}

Vec3d Patch::reflectInPlane(Vec3d v, Vec3d n) {
    n.normalize();
    float k = v.dot(n);
    v -= n*(k*2);
    return v;
}

VRGeometryPtr Patch::makeTrianglePlane(int N, bool wire) {
    VRGeoData data;

    // vertices
    for(int i=0;i<N+1;i++) {
        for(int j=0;j<N+1-i;j++) {
            Vec3d pos((float)i/(N+1),(float)j/(N+1),0);
            data.pushVert( pos, Vec3d(0,0,1), Vec2d(0,0) );
        }
    }

    //indizierung
    int k = 0;
    for(int i=0;i<N;i++) {
        for(int j=0;j<N-i;j++) {
            data.pushTri(k+j, k+j+1, k+j+N-i+1);
            if (j != N-1-i) data.pushTri(k+j+1, k+j+N-i+2, k+j+N-i+1);
        }
        k += N+1-i;
    }

    auto mat = VRMaterial::get("subPatchMat");
    mat->setDiffuse(Color3f(0,1,0));
    mat->setAmbient(Color3f(0.4, 0.4, 0.2));
    mat->setLit(0);
    if (wire) mat->setWireFrame(true);

    auto geo = data.asGeometry("subpatch");
    geo->setMaterial(mat);
    return geo;
}

void Patch::calcBezQuadPlane(bezVRPolygon<4>& q) {
    //schrittweite
    float step = 1./(q.N-1);

    //hilfspunkte ausen und innen
    Vec3d ha[8];
    Vec3d hi[4];
    //kanten
    Vec3d r[4];

    r[0] = q.p[1]-q.p[0];
    r[1] = q.p[3]-q.p[2];
    r[2] = q.p[2]-q.p[0];
    r[3] = q.p[3]-q.p[1];

    //float rL[4];
    //for (int i=0;i<4;i++) rL[i] = r[i].length();

    if (r[0] != Vec3d(0,0,0)) {
        ha[0] = q.p[0]+projectInPlane(r[0]*(1./3.),q.n[0],false);
        ha[1] = q.p[1]+projectInPlane(-r[0]*(1./3.),q.n[1],false);
    } else {
        ha[0] = q.p[0];
        ha[1] = q.p[1];
    }

    if (r[1] != Vec3d(0,0,0)) {
        ha[2] = q.p[2]+projectInPlane(r[1]*(1./3.),q.n[2],false);
        ha[3] = q.p[3]+projectInPlane(-r[1]*(1./3.),q.n[3],false);
    } else {
        ha[2] = q.p[2];
        ha[3] = q.p[3];
    }

    if (r[2] != Vec3d(0,0,0)) {
        ha[4] = q.p[0]+projectInPlane(r[2]*(1./3.),q.n[0],false);
        ha[5] = q.p[2]+projectInPlane(-r[2]*(1./3.),q.n[2],false);
    } else {
        ha[4] = q.p[0];
        ha[5] = q.p[2];
    }

    if (r[3] != Vec3d(0,0,0)) {
        ha[6] = q.p[1]+projectInPlane(r[3]*(1./3.),q.n[1],false);
        ha[7] = q.p[3]+projectInPlane(-r[3]*(1./3.),q.n[3],false);
    } else {
        ha[6] = q.p[1];
        ha[7] = q.p[3];
    }

    //---V1
    Vec3d temp[2];
    temp[0] = ha[2]-ha[0];//zwischenrechnungen
    temp[1] = ha[3]-ha[1];
    hi[0] = ha[0]+projectInPlane(temp[0]*(1./3),q.n[0],false);
    hi[1] = ha[1]+projectInPlane(temp[1]*(1./3),q.n[1],false);
    hi[2] = ha[2]+projectInPlane(-temp[0]*(1./3),q.n[2],false);
    hi[3] = ha[3]+projectInPlane(-temp[1]*(1./3),q.n[3],false);

    //---V2
    /*hi[0] = ha[0]+ha[4]-q.p[0];
    hi[1] = ha[1]+ha[6]-q.p[1];
    hi[2] = ha[2]+ha[5]-q.p[2];
    hi[3] = ha[3]+ha[7]-q.p[3];*/

    //---V3 -b
    /*temp[0] = ha[2]-ha[0];//zwischenrechnungen
    temp[1] = ha[3]-ha[1];
    hi[0] = q.p[0]+projectInPlane(temp[0]*(1./3),q.n[0]);
    hi[1] = ha[1]+ha[6]-q.p[1];
    hi[2] = ha[2]+ha[5]-q.p[2];
    hi[3] = ha[3]+ha[7]-q.p[3];*/

    //forward diff
    Vec3d DELs[3];
    Vec3d DELt[4][3];

    //hilfskoefficienten

    vector<Vec3d> T[4];
	for (int i = 0; i < 4; i++) T[i].resize(q.N);
    Vec3d G[4][4];
    Vec3d P[4][4];
    Vec3d bs[3];

    //eckpunkte in die punktmatrix
    P[0][0] = q.p[0];
    P[0][3] = q.p[1];
    P[3][0] = q.p[2];
    P[3][3] = q.p[3];
    //hilfspunkte
    P[0][1] = ha[0];
    P[0][2] = ha[1];
    P[1][0] = ha[4];
    P[1][1] = hi[0];
    P[1][2] = hi[1];
    P[1][3] = ha[6];
    P[2][0] = ha[5];
    P[2][1] = hi[2];
    P[2][2] = hi[3];
    P[2][3] = ha[7];
    P[3][1] = ha[2];
    P[3][2] = ha[3];

    //koeffizienten der t-polynome
    for (int i=0; i<4;i++) {
        G[i][0] = P[i][3]-P[i][0]+P[i][1]*3-P[i][2]*3;
        G[i][1] = P[i][0]*3-P[i][1]*6+P[i][2]*3;
        G[i][2] = P[i][1]*3-P[i][0]*3;
        G[i][3] = P[i][0];

        //berechne schritte
        DELt[i][2] = G[i][0]*6*step*step*step;
        DELt[i][1] = G[i][1]*2*step*step + DELt[i][2];
        DELt[i][0] = G[i][2]*step + G[i][1]*step*step + DELt[i][2]*(1./6.);
        T[i][0] = G[i][3];
    }

    for (int i=1;i<q.N-1;i++) {

        for (int k=0; k<4;k++) {
            T[k][i] = T[k][i-1]+DELt[k][0];
            DELt[k][0] += DELt[k][1];
            DELt[k][1] += DELt[k][2];
        }

        //berechne schritte
        //berechne die koeffizienten -> p[0]+b[0]t+b[1]t²+b[2]t³
        bs[0] = T[1][i]*3-T[0][i]*3;
        bs[1] = T[0][i]*3+T[2][i]*3-T[1][i]*6;
        bs[2] = T[1][i]*3-T[0][i]-T[2][i]*3+T[3][i];
        //berechne schritte
        DELs[2] = bs[2]*6*step*step*step;
        DELs[1] = bs[1]*2*step*step + DELs[2];
        DELs[0] = bs[0]*step + bs[1]*step*step + DELs[2]*(1./6.);

        for (int j=1;j<q.N-1;j++) {
            //q.map[i][j] = q.map[i][j-1]+DELs[0]; ----DIREKTER ZUGRIFF
            DELs[0] += DELs[1];
            DELs[1] += DELs[2];
        }
    }
}

void Patch::calcBezTrianglePlane(bezVRPolygon<3>& q) {
    cout << "calcBezTrianglePlane\n";
    //schrittweite
    float step = 1./(q.N-1);

    //normalize normals, just to be safe
    for (int i=0;i<3;i++) q.n[i].normalize();

    //Punkte Matrix4d
    Vec3d P[4][4][4];

    //kanten
    Vec3d r[3];
    r[0] = q.p[1]-q.p[0];
    r[1] = q.p[0]-q.p[2];
    r[2] = q.p[2]-q.p[1];

    //kanten längen
    float rL[3];
    for (int i=0;i<3;i++) rL[i] = r[i].length();

    for (int i=0;i<3;i++) {
        if (rL[i] == 0) {
            cout << "\n\nError! No triangle!";
            return;
        }
        if (q.n[i] == Vec3d(0,0,0)) {
            cout << "\n\nError! No normal vector!";
            return;
        }
    }

    //ecken
    P[3][0][0] = q.p[0];
    P[0][3][0] = q.p[1];
    P[0][0][3] = q.p[2];

    //Hilfspunkte auf kanten
    P[2][1][0] = P[3][0][0]+projectInPlane( r[0]*(1./3.),q.n[0],false);
    P[1][2][0] = P[0][3][0]+projectInPlane(-r[0]*(1./3.),q.n[1],false);

    P[2][0][1] = P[3][0][0]+projectInPlane(-r[1]*(1./3.),q.n[0],false);
    P[1][0][2] = P[0][0][3]+projectInPlane( r[1]*(1./3.),q.n[2],false);

    P[0][2][1] = P[0][3][0]+projectInPlane( r[2]*(1./3.),q.n[1],false);
    P[0][1][2] = P[0][0][3]+projectInPlane(-r[2]*(1./3.),q.n[2],false);

    //berechne die distanzen zwischen den hilfspunkten
    Vec3d hr[3];
    hr[0] = P[1][2][0]-P[2][1][0];
    hr[1] = P[0][1][2]-P[0][2][1];
    hr[2] = P[2][0][1]-P[1][0][2];

    //berechne die mittleren normalvectoren
    Vec3d n[3];
    n[0] = reflectInPlane(q.n[0]+q.n[1], r[0]);
    n[1] = reflectInPlane(q.n[1]+q.n[2], r[2]);
    n[2] = reflectInPlane(q.n[2]+q.n[0], r[1]);

    for (int l=0;l<3;l++) n[l] = hr[l].cross(n[l].cross(hr[l]));
    for (int l=0;l<3;l++) n[l].normalize();

    //berechne den schnittpunkt der 3 ebenen
    Matrix4d A = Matrix4d(  n[0][0], n[0][1], n[0][2], 0,
                        n[1][0], n[1][1], n[1][2], 0,
                        n[2][0], n[2][1], n[2][2], 0,
                        0,       0,       0,       1);
    Vec4d b = Vec4d(n[0].dot(P[2][1][0]), n[1].dot(P[0][2][1]), n[2].dot(P[1][0][2]), 0);
    Matrix4d Ai;

    if (abs(A.det()) > 0.0001) {
        for (int l=0;l<3;l++) {
            Ai = A;
            Ai[l] = b;
            P[1][1][1][l] = Ai.det()/A.det();
        }
    } else {
        //cout << "\n det = 0";
        float h = -r[0].dot(r[1])/r[1].dot(r[1]);
        P[1][1][1] = (P[1][2][0] + P[1][0][2]
                    + (P[0][1][2]*2-P[0][2][1]-P[0][0][3])*h
                    + (P[0][2][1]*2-P[0][3][0]-P[0][1][2])*(1-h))*0.5;
    }

    //forward diff
    q.geo = makeTrianglePlane(q.N-1,q.wired);

    Vec3d DELu[3];
    Vec3d Bu[4];
    Vec3d DELv[3][3];
    Vec3d Bv[4][4];
    Vec3d T[3][q.N];

    //Bv03 * v³ + Bv02 * v² + Bv01 * v + Bv00 = Bu0
    Bv[0][0] = P[0][0][3];
    Bv[0][1] = (P[0][1][2] - P[0][0][3])*3;
    Bv[0][2] = -P[0][1][2]*6 + (P[0][2][1]  + P[0][0][3])*3;
    Bv[0][3] = P[0][3][0] - P[0][0][3] + (P[0][1][2] - P[0][2][1])*3 ;

    //u * [Bv13 * v³ + Bv12 * v² + Bv11 * v + Bv10] = Bu1
    Bv[1][0] = (P[1][0][2] - P[0][0][3])*3;
    Bv[1][1] = (P[0][0][3] - P[1][0][2] + P[1][1][1] - P[0][1][2])*6;
    Bv[1][2] = (P[1][2][0] + P[1][0][2] - P[0][2][1] - P[0][0][3])*3 + (P[0][1][2] - P[1][1][1])*6;
    Bv[1][3] = Vec3d();

    //u² * [Bv23 * v³ + Bv22 * v² + Bv21 * v + Bv20] = Bu2
    Bv[2][0] = (P[2][0][1] + P[0][0][3])*3 - P[1][0][2]*6;
    Bv[2][1] = (P[2][1][0] - P[2][0][1] + P[0][1][2] - P[0][0][3])*3 + (P[1][0][2] - P[1][1][1])*6;
    Bv[2][2] = Vec3d();
    Bv[2][3] = Vec3d();

    //u³ * [Bv33 * v³ + Bv32 * v² + Bv31 * v + Bv30] = Bu3
    Bv[3][0] = P[3][0][0] - P[0][0][3] + (P[1][0][2] - P[2][0][1])*3;
    Bv[3][1] = Vec3d();
    Bv[3][2] = Vec3d();
    Bv[3][3] = Vec3d();

    //berechne schritte
    DELv[0][2] = Bv[0][3]*6*step*step*step;
    DELv[0][1] = Bv[0][2]*2*step*step + DELv[0][2];
    DELv[0][0] = Bv[0][1]*step + Bv[0][2]*step*step + DELv[0][2]*(1./6.);

    DELv[1][2] = Vec3d(0,0,0);
    DELv[1][1] = Bv[1][2]*2*step*step;
    DELv[1][0] = Bv[1][1]*step + Bv[1][2]*step*step;

    DELv[2][2] = Vec3d(0,0,0);
    DELv[2][1] = Vec3d(0,0,0);
    DELv[2][0] = Bv[2][1]*step;

    T[0][0] = Bv[0][0];
    T[1][0] = Bv[1][0];
    T[2][0] = Bv[2][0];

    GeoPnt3fPropertyMTRecPtr Pos = (GeoPnt3fProperty*)q.geo->getMesh()->geo->getPositions();
    if (!Pos) { cout << "AAAA, pos invalid\n"; return; }

    int iter = 0;
    for (int i=0;i<q.N;i++) {
        if (iter >= Pos->size()) { cout << "AA " << iter << endl; break; }
        //cout << "Ti " << T[0][i] << ",    " << T[1][i] << ",    " << T[2][i] << endl;

        for (int k=0; k<3 && i>0;k++) {
            T[k][i] = T[k][i-1]+DELv[k][0];
            DELv[k][0] += DELv[k][1];
            DELv[k][1] += DELv[k][2];
        }

        //Bu3 * u³ + Bu2 * u² + Bu1 * u + Bu0
        Bu[3] = Bv[3][0];
        Bu[2] = T[2][i];
        Bu[1] = T[1][i];
        Bu[0] = T[0][i];

        //berechne schritte
        DELu[2] = Bu[3]*6*step*step*step;
        DELu[1] = Bu[2]*2*step*step + DELu[2];
        DELu[0] = Bu[1]*step + Bu[2]*step*step + DELu[2]*(1./6.);

        //cout << " Bu " << Bu[0] << ", " << Bu[1] << ", " << Bu[2] << ", " << Bu[3] << endl;
        //cout << " DELu " << DELu[0] << ", " << DELu[1] << ", " << DELu[2] << endl;

        Pos->setValue(Bu[0], iter);
        iter++;
        for (int j=1;j<q.N-i;j++) {
            if (iter >= Pos->size()) { cout << "BB " << iter << endl; break; }
            Pnt3f p0 = Pos->getValue(iter);
            Pos->setValue(Vec3d(Pos->getValue(iter-1))+DELu[0], iter);
            //cout << " B " << p0 << " -> " << Pos->getValue(iter) << "   " << DELu[0] << endl;
            iter++;
            DELu[0] += DELu[1];
            DELu[1] += DELu[2];
        }
    }

    //calculate the normals------------------------------------------------------------
    GeoVec3fPropertyMTRecPtr Norms = (GeoVec3fProperty*)q.geo->getMesh()->geo->getNormals();

    P[2][0][0] = q.n[0];
    P[0][2][0] = q.n[1];
    P[0][0][2] = q.n[2];

    P[1][1][0] = reflectInPlane((P[2][0][0]+P[0][2][0])*0.5, P[3][0][0]-P[0][3][0]);
    P[1][1][0].normalize();
    P[1][0][1] = reflectInPlane((P[2][0][0]+P[0][0][2])*0.5, P[3][0][0]-P[0][0][3]);
    P[1][0][1].normalize();
    P[0][1][1] = reflectInPlane((P[0][2][0]+P[0][0][2])*0.5, P[0][3][0]-P[0][0][3]);
    P[0][1][1].normalize();

    //Bv02 * v² + Bv01 * v + Bv00 = Bu0
    Bv[0][0] = P[0][0][2];
    Bv[0][1] = P[0][1][1]*2 - P[0][0][2]*2;
    Bv[0][2] = P[0][2][0] + P[0][0][2] - P[0][1][1]*2;

    //u * [Bv12 * v² + Bv11 * v + Bv10] = Bu1
    Bv[1][0] = P[1][0][1]*2 - P[0][0][2]*2;
    Bv[1][1] = P[0][0][2]*2 + P[1][1][0]*2 - P[1][0][1]*2 - P[0][1][1]*2;

    //u² * [Bv22 * v² + Bv21 * v + Bv20] = Bu2
    Bv[2][0] = P[2][0][0] + P[0][0][2] - P[1][0][1]*2;

    //berechne schritte
    DELv[0][1] = Bv[0][2]*2*step*step;
    DELv[0][0] = Bv[0][1]*step + Bv[0][2]*step*step;

    DELv[1][0] = Bv[1][1]*step;

    T[0][0] = Bv[0][0];
    T[1][0] = Bv[1][0];

    //Bw2 * w² + Bw1 * w + Bw0
    Vec3d Bw[3];
    Vec3d DELw[2];
    Vec3d Tw;
    Bw[0] = P[2][0][0];
    Bw[1] = (P[1][1][0]-P[2][0][0])*2;
    Bw[2] = P[0][2][0] + P[2][0][0] - P[1][1][0]*2;
    DELw[1] = Bw[2]*2*step*step;
    DELw[0] = Bw[1]*step + Bw[2]*step*step;
    Tw = Bw[0];

    iter = 0;
    for (int i=0;i<q.N;i++) {
        if (i>0) {
            T[0][i] = T[0][i-1]+DELv[0][0];
            T[1][i] = T[1][i-1]+DELv[1][0];
            DELv[0][0] += DELv[0][1];
        }

        //Bu2 * u² + Bu1 * u + Bu0
        Bu[2] = Bv[2][0];
        Bu[1] = T[1][i];
        Bu[0] = T[0][i];

        //berechne schritte
        DELu[1] = Bu[2]*2*step*step;
        DELu[0] = Bu[1]*step + Bu[2]*step*step;
        Norms->setValue(Bu[0], iter);
        iter++;
        for (int j=1;j<q.N-i;j++) {
            Vec3d temp = Vec3d(Norms->getValue(iter-1)) + DELu[0];
            Norms->setValue(temp, iter);
            DELu[0] += DELu[1];

            iter++;
        }
    }

    //calc texture coords---------------------------------------------------------
    //nicht getestet
    GeoVec2fPropertyMTRecPtr Texs = (GeoVec2fProperty*)q.geo->getMesh()->geo->getTexCoords();

    //Baustelle
    Vec2d r1 = (q.tex[1]-q.tex[2])*step;
    Vec2d r2 = (q.tex[0]-q.tex[2])*step;

    iter=0;
    for (int i=0;i<q.N;i++) {
        for (int j=0;j<q.N-i;j++) {
            Vec2d temp = q.tex[2] + r1*i + r2*j;
            Texs->setValue(temp, iter);
            iter++;
        }
    }
}

//iteriert über die flächen der geometrie und macht bezierflächen hin
VRObjectPtr Patch::fromGeometry(VRGeometryPtr geo, int N, bool wire) {
    auto obj = VRObject::create("patch");
    auto g = geo->getMesh()->geo;
    for (TriangleIterator it = g->beginTriangles(); it != g->endTriangles(); ++it) {
        bezVRPolygon<3> q;
        q.N = N;
        q.wired = wire;
        q.p[0] = Vec3d( it.getPosition(0) );
        q.p[1] = Vec3d( it.getPosition(1) );
        q.p[2] = Vec3d( it.getPosition(2) );
        q.n[0] = Vec3d( it.getNormal(0) );
        q.n[1] = Vec3d( it.getNormal(1) );
        q.n[2] = Vec3d( it.getNormal(2) );
        q.tex[0] = Vec2d( it.getTexCoords(0) );
        q.tex[1] = Vec2d( it.getTexCoords(1) );
        q.tex[2] = Vec2d( it.getTexCoords(2) );

        calcBezTrianglePlane(q);
        obj->addChild(q.geo);
    }
    return obj;
}


