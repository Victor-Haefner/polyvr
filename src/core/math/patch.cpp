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


Vec3d Patch::projectInPlane(Vec3d v, Vec3d n, bool keep_length, bool normalize) {
    if (normalize) n.normalize();
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

VRGeometryPtr Patch::makeQuadPlane(int N, bool wire) {
    VRGeoData data;

    // vertices
    for(int i=0;i<N+1;i++) {
        for(int j=0;j<N+1;j++) {
            Vec3d pos((float)i/(N+1),(float)j/(N+1),0);
            data.pushVert( pos, Vec3d(0,0,1), Vec2d(0,0) );
        }
    }

    //indizierung
    int k = 0;
    for(int i=0;i<N;i++) {
        for(int j=0;j<N;j++) {
            data.pushQuad(k+j, k+j+1, k+j+N+2, k+j+N+1);
            //if (j != N-1-i) data.pushQuad(k+j+1, k+j+N-i+2, k+j+N-i+1);
        }
        k += N+1;
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

// from bezier to cubic polynomial
template<typename T>
void computePolynomialFactors(T& A, T& B, T& C, T& D, const T& P0, const T& P1, const T& P2, const T& P3) {
    A = P3 -P0 +(P1-P2)*3;
    B = (P0-P1*2+P2)*3;
    C = (P1-P0)*3;
    D = P0;
}

void Patch::calcBezQuadPlane(bezVRPolygon<4>& q, vector<Vec3d> handles, bool normalizeNorms) {
    cout << "calcBezQuadPlane" << endl;
    //schrittweite
    float step = 1./(q.N-1);

    //normalize normals, just to be safe
    if (normalizeNorms)
        for (int i=0;i<4;i++) q.n[i].normalize();

    for (int i=0;i<4;i++) cout << "  --- " << q.n[i] << endl;

    //hilfspunkte ausen und innen
    Vec3d ha[8];
    Vec3d hi[4];
    //kanten
    Vec3d r[4];

    r[0] = q.p[1]-q.p[0];
    r[1] = q.p[2]-q.p[3];
    r[2] = q.p[3]-q.p[0];
    r[3] = q.p[2]-q.p[1];

    //float rL[4];
    //for (int i=0;i<4;i++) rL[i] = r[i].length();

    if (handles.size() != 12) {
        if (r[0] != Vec3d(0,0,0)) {
            ha[0] = q.p[0]+projectInPlane(r[0]*(1./3.),q.n[0],false,normalizeNorms);
            ha[1] = q.p[1]+projectInPlane(-r[0]*(1./3.),q.n[1],false,normalizeNorms);
        } else {
            ha[0] = q.p[0];
            ha[1] = q.p[1];
        }

        if (r[1] != Vec3d(0,0,0)) {
            ha[2] = q.p[3]+projectInPlane(r[1]*(1./3.),q.n[3],false,normalizeNorms);
            ha[3] = q.p[2]+projectInPlane(-r[1]*(1./3.),q.n[2],false,normalizeNorms);
        } else {
            ha[2] = q.p[3];
            ha[3] = q.p[2];
        }

        if (r[2] != Vec3d(0,0,0)) {
            ha[4] = q.p[0]+projectInPlane(r[2]*(1./3.),q.n[0],false,normalizeNorms);
            ha[5] = q.p[3]+projectInPlane(-r[2]*(1./3.),q.n[3],false,normalizeNorms);
        } else {
            ha[4] = q.p[0];
            ha[5] = q.p[3];
        }

        if (r[3] != Vec3d(0,0,0)) {
            ha[6] = q.p[1]+projectInPlane(r[3]*(1./3.),q.n[1],false,normalizeNorms);
            ha[7] = q.p[2]+projectInPlane(-r[3]*(1./3.),q.n[2],false,normalizeNorms);
        } else {
            ha[6] = q.p[1];
            ha[7] = q.p[2];
        }

        //---V1
        Vec3d temp[2];
        temp[0] = ha[2]-ha[0];//zwischenrechnungen
        temp[1] = ha[3]-ha[1];
        hi[0] = ha[0]+projectInPlane(temp[0]*(1./3),q.n[0],false,normalizeNorms);
        hi[1] = ha[1]+projectInPlane(temp[1]*(1./3),q.n[1],false,normalizeNorms);
        hi[2] = ha[2]+projectInPlane(-temp[0]*(1./3),q.n[3],false,normalizeNorms);
        hi[3] = ha[3]+projectInPlane(-temp[1]*(1./3),q.n[2],false,normalizeNorms);
    } else {
        ha[0] = handles[0];
        ha[1] = handles[1];
        ha[2] = handles[2];
        ha[3] = handles[3];

        ha[4] = handles[4];
        ha[5] = handles[5];
        ha[6] = handles[6];
        ha[7] = handles[7];

        hi[0] = handles[8];
        hi[1] = handles[9];
        hi[2] = handles[10];
        hi[3] = handles[11];
    }

    //forward diff
    q.geo = makeQuadPlane(q.N-1,q.wired);

    //forward diff
    Vec3d DELs[3];
    Vec3d DELt[4][3];

    //hilfskoefficienten

    vector<Vec3d> T[4];
	for (int i = 0; i < 4; i++) T[i].resize(q.N);
    Vec3d G[4][4];
    Vec3d P[4][4];
    Vec3d bs[4];

    //eckpunkte in die punktmatrix
    P[0][0] = q.p[0];
    P[0][3] = q.p[1];
    P[3][0] = q.p[3];
    P[3][3] = q.p[2];
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
    for (int i=0; i<4; i++) {
        computePolynomialFactors(G[i][0], G[i][1], G[i][2], G[i][3],  P[i][0], P[i][1], P[i][2], P[i][3]);

        //berechne schritte
        DELt[i][2] = G[i][0]*6*step*step*step;
        DELt[i][1] = G[i][1]*2*step*step + DELt[i][2];
        DELt[i][0] = G[i][2]*step + G[i][1]*step*step + DELt[i][2]*(1./6.);
        T[i][0] = G[i][3];
    }

    GeoPnt3fPropertyMTRecPtr Pos = (GeoPnt3fProperty*)q.geo->getMesh()->geo->getPositions();
    if (!Pos) { cout << "AAAA, pos invalid\n"; return; }

    int iter = 0;
    for (int i=0; i<q.N; i++) {
        if (iter >= (int)Pos->size()) { cout << "AA " << iter << endl; break; }

        for (int k=0; k<4 && i>0; k++) {
            T[k][i] = T[k][i-1]+DELt[k][0];
            DELt[k][0] += DELt[k][1];
            DELt[k][1] += DELt[k][2];
        }

        //bs3 * t³ + bs2 * t² + bs1 * t + bs0
        computePolynomialFactors(bs[3], bs[2], bs[1], bs[0],  T[0][i], T[1][i], T[2][i], T[3][i]);

        //berechne schritte
        DELs[2] = bs[3]*6*step*step*step;
        DELs[1] = bs[2]*2*step*step + DELs[2];
        DELs[0] = bs[1]*step + bs[2]*step*step + DELs[2]*(1./6.);

        Pos->setValue(bs[0], iter);
        iter++;
        for (int j=1;j<q.N;j++) {
            if (iter >= (int)Pos->size()) { cout << "BB " << iter << endl; break; }
            //q.map[i][j] = q.map[i][j-1]+DELs[0]; ----DIREKTER ZUGRIFF
            Pos->setValue(Vec3d(Pos->getValue(iter-1))+DELs[0], iter);
            iter++;
            DELs[0] += DELs[1];
            DELs[1] += DELs[2];
        }
    }
}

void Patch::calcBezTrianglePlane(bezVRPolygon<3>& q, bool normalizeNorms) {
    cout << "calcBezTrianglePlane" << endl;
    //schrittweite
    float step = 1./(q.N-1);

    //normalize normals, just to be safe
    if (normalizeNorms)
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
    P[2][1][0] = P[3][0][0]+projectInPlane( r[0]*(1./3.),q.n[0],false,normalizeNorms);
    P[1][2][0] = P[0][3][0]+projectInPlane(-r[0]*(1./3.),q.n[1],false,normalizeNorms);

    P[2][0][1] = P[3][0][0]+projectInPlane(-r[1]*(1./3.),q.n[0],false,normalizeNorms);
    P[1][0][2] = P[0][0][3]+projectInPlane( r[1]*(1./3.),q.n[2],false,normalizeNorms);

    P[0][2][1] = P[0][3][0]+projectInPlane( r[2]*(1./3.),q.n[1],false,normalizeNorms);
    P[0][1][2] = P[0][0][3]+projectInPlane(-r[2]*(1./3.),q.n[2],false,normalizeNorms);

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
	vector<Vec3d> T[3];
	for (int i = 0; i < 3; i++) T[i] = vector<Vec3d>(q.N);

    //Bv03 * v³ + Bv02 * v² + Bv01 * v + Bv00 = Bu0
    /*Bv[0][0] = P[0][0][3];
    Bv[0][1] = (P[0][1][2] - P[0][0][3])*3;
    Bv[0][2] = -P[0][1][2]*6 + (P[0][2][1]  + P[0][0][3])*3;
    Bv[0][3] = P[0][3][0] - P[0][0][3] + (P[0][1][2] - P[0][2][1])*3;*/
    computePolynomialFactors(Bv[0][3], Bv[0][2], Bv[0][1], Bv[0][0],  P[0][0][3], P[0][1][2], P[0][2][1], P[0][3][0]);

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
    for (int i=0; i<q.N; i++) {
        if (iter >= (int)Pos->size()) { cout << "AA " << iter << endl; break; }
        //cout << "Ti " << T[0][i] << ",    " << T[1][i] << ",    " << T[2][i] << endl;

        for (int k=0; k<3 && i>0; k++) {
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

        Pos->setValue(Bu[0], iter);
        iter++;
        for (int j=1; j+i<q.N; j++) {
            if (iter >= (int)Pos->size()) { cout << "BB " << iter << endl; break; }
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

VRObjectPtr Patch::getSurface() { return object.lock(); }

VRObjectPtr Patch::fromTriangle(vector<Vec3d> positions, vector<Vec3d> normals, int N, bool wire) {
    auto obj = VRObject::create("patch");
    if (positions.size() != 3 || normals.size() != 3) return obj;

    bezVRPolygon<3> q;
    q.N = N;
    q.wired = wire;
    q.p[0] = positions[0];
    q.p[1] = positions[1];
    q.p[2] = positions[2];
    q.n[0] = normals[0];
    q.n[1] = normals[1];
    q.n[2] = normals[2];

    calcBezTrianglePlane(q, false);
    obj->addChild(q.geo);
    object = obj;
    return obj;
}

VRObjectPtr Patch::fromFullQuad(vector<Vec3d> positions, vector<Vec3d> normals, vector<Vec3d> handles, int N, bool wire) {
    auto obj = VRObject::create("patch");
    if (positions.size() != 4 || normals.size() != 4) return obj;

    bezVRPolygon<4> q;
    q.N = N;
    q.wired = wire;
    q.p[0] = positions[0];
    q.p[1] = positions[1];
    q.p[2] = positions[2];
    q.p[3] = positions[3];
    q.n[0] = normals[0];
    q.n[1] = normals[1];
    q.n[2] = normals[2];
    q.n[3] = normals[3];

    calcBezQuadPlane(q, handles, false);
    obj->addChild(q.geo);
    object = obj;
    return obj;
}

VRObjectPtr Patch::fromQuad(vector<Vec3d> positions, vector<Vec3d> normals, int N, bool wire) {
    return fromFullQuad(positions, normals, vector<Vec3d>(), N, wire);
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

        calcBezTrianglePlane(q, true);
        obj->addChild(q.geo);
    }
    object = obj;
    return obj;
}

float projectEdge(const Vec3d& p, const Vec3d& pe, const Vec3d& de) {
    Vec3d d = p-pe;
    float L2 = de.squareLength();
    return d.dot(de)/L2;
}

bool abovePlane(const Vec3d& p, const Vec3d& P, const Vec3d& n) {
    return (p-P).dot(n) >= 0;
}

Vec3d getClosestOnTriangle(Vec3d p, Vec3d& n, const Vec3d& p1, const Vec3d& p2, const Vec3d& p3) {
    Vec3d d12 = p2-p1;
    Vec3d d31 = p1-p3;
    Vec3d d23 = p3-p2;
    n = d23.cross(d12);

    float u12 = projectEdge(p, p1, d12);
    float u31 = projectEdge(p, p3, d31);
    if (u31 > 1 && u12 < 0) return p1;

    float u23 = projectEdge(p, p2, d23);
    if (u12 > 1 && u23 < 0) return p2;
    if (u23 > 1 && u31 < 0) return p3;

    Vec3d n12 = d12.cross(n);
    Vec3d n23 = d23.cross(n);
    Vec3d n31 = d31.cross(n);

    if ( u12 <=1 && u12 >= 0 && !abovePlane( p,p1,n12 )) return p1 + d12*u12;
    if ( u23 <=1 && u23 >= 0 && !abovePlane( p,p2,n23 )) return p2 + d23*u23;
    if ( u31 <=1 && u31 >= 0 && !abovePlane( p,p3,n31 )) return p3 + d31*u31;

    // return projected point on triangle
    n.normalize();
    return p - (p-p1).dot(n)*n;
}

PosePtr Patch::getClosestPose(Vec3d p) {
    float dmin = 1e6;
    //float _3 = 1.0/3;
    PosePtr Dmin = Pose::create();
    auto obj = object.lock();
    if (obj) {
        for (auto o : obj->getChildren()) {
            auto geo = dynamic_pointer_cast<VRGeometry>(o);
            if (!geo) continue;

            auto g = geo->getMesh()->geo;
            for (TriangleIterator it = g->beginTriangles(); it != g->endTriangles(); ++it) {
                Vec3d p0 = Vec3d( it.getPosition(0) );
                Vec3d p1 = Vec3d( it.getPosition(1) );
                Vec3d p2 = Vec3d( it.getPosition(2) );

                //Vec3d pn = (p0+p1+p2)*_3; // approx with triangle center
                Vec3d n;
                Vec3d pn = getClosestOnTriangle(p,n,p0,p1,p2);

                float d = (p-pn).length();
                if (d < dmin) {
                    dmin = d; // get min dist
                    Dmin->setPos(pn);
                    Dmin->setDir(n);
                }
            }
        }
    }
    return Dmin;
}

Vec3d Patch::getClosestPoint(Vec3d p) {
    float dmin = 1e6;
    //float _3 = 1.0/3;
    Vec3d Dmin;
    auto obj = object.lock();
    if (obj) {
        for (auto o : obj->getChildren()) {
            auto geo = dynamic_pointer_cast<VRGeometry>(o);
            if (!geo) continue;

            auto g = geo->getMesh()->geo;
            for (TriangleIterator it = g->beginTriangles(); it != g->endTriangles(); ++it) {
                Vec3d p0 = Vec3d( it.getPosition(0) );
                Vec3d p1 = Vec3d( it.getPosition(1) );
                Vec3d p2 = Vec3d( it.getPosition(2) );

                //Vec3d pn = (p0+p1+p2)*_3; // approx with triangle center
                Vec3d n;
                Vec3d pn = getClosestOnTriangle(p,n,p0,p1,p2);

                float d = (p-pn).length();
                if (d < dmin) {
                    dmin = d; // get min dist
                    Dmin = pn;
                }
            }
        }
    }
    return Dmin;
}

float Patch::getDistance(Vec3d p) {
    Vec3d c = getClosestPoint(p);
    return (c-p).length();
}


