#include "patch.h"

#include <OpenSG/OSGFaceIterator.h>
#include <OpenSG/OSGTriangleIterator.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

//TODO



template <int S>
patch::bezPolygon<S>::bezPolygon() {
    initPolygon();
}

template <int S>
patch::bezPolygon<S>::~bezPolygon() {
    delete[] p;
    delete[] n;
    delete[] tex;
}

template <int S>
void patch::bezPolygon<S>::initPolygon() {
    geo = 0;
    N = 0;
    wired = false;

    p = new Vec3f[S];
    n = new Vec3f[S];
    tex = new Vec2f[S];

    for (int i=0;i<S;i++) {
        for (int j=0;j<3;j++){
            this->p[i][j]=0;
            this->n[i][j]=0;
            if (j<2) this->tex[i][j]=0;
        }
    }
}


Vec3f patch::projectInPlane(Vec3f v, Vec3f n, bool keep_length) {
    n.normalize();
    float l;
    if (keep_length) l = v.length();
    float k = v.dot(n);
    v -= k*n;
    if (keep_length) v = v*(l/v.length());
    return v;
}

Vec3f patch::reflectInPlane(Vec3f v, Vec3f n) {
    n.normalize();
    float k = v.dot(n);
    v -= k*n*2;
    return v;
}

/*GeometryMTRecPtr patch::makeTrianglePlaneGeo(int N, bool wire = false) {
    GeoPTypesPtr                Type = GeoPTypesUI8::create();
    GeoPLengthsPtr              Length = GeoPLengthsUI32::create();
    GeoPositions3fPtr           Pos = GeoPositions3f::create();
    GeoNormals3fPtr             Norms = GeoNormals3f::create();
    GeoIndicesUI32Ptr           Indices = GeoIndicesUI32::create();
    SimpleTexturedMaterialPtr   Mat = SimpleTexturedMaterial::create();
    GeoTexCoords2fPtr           Tex = GeoTexCoords2f::create();

    beginEditCP(Type, GeoPTypesUI8::GeoPropDataFieldMask);
    beginEditCP(Length, GeoPLengthsUI32::GeoPropDataFieldMask);
    beginEditCP(Pos, GeoPositions3f::GeoPropDataFieldMask);
    beginEditCP(Norms, GeoNormals3f::GeoPropDataFieldMask);
    beginEditCP(Indices, GeoIndicesUI32::GeoPropDataFieldMask);
    beginEditCP(Tex, GeoTexCoords2f::GeoPropDataFieldMask);
    beginEditCP(Mat);
        int tN = N*N;
        Type->addValue(GL_TRIANGLES);
        Length->addValue(3*tN);

        //positionen und Normalen
        int PosCount = 0;
        int NormsCount = 0;
        for(int i=0;i<N+1;i++) {
            for(int j=0;j<N+1-i;j++) {
                PosCount++;
                NormsCount++;
                Pos->addValue(Vec3f((float)i/(N+1),(float)j/(N+1),0));
                Norms->addValue(Vec3f(0,0,1));
                Tex->addValue(Vec2f(0,0));
            }
        }
        //indizierung
        int k=0;
        for(int i=0;i<N;i++) {
            for(int j=0;j<N-i;j++) {
                Indices->addValue(k+j);
                Indices->addValue(k+j+1);
                Indices->addValue(k+j+N-i+1);
                if (j != N-1-i) {
                    Indices->addValue(k+j+1);
                    Indices->addValue(k+j+N-i+2);
                    Indices->addValue(k+j+N-i+1);
                }
            }
            k+=N+1-i;
        }

        Mat->setDiffuse(Color3f(0.8,0.8,0.6));
        Mat->setAmbient(Color3f(0.4, 0.4, 0.2));
        //Mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    endEditCP(Type, GeoPTypesUI8::GeoPropDataFieldMask);
    endEditCP(Length, GeoPLengthsUI32::GeoPropDataFieldMask);
    endEditCP(Pos, GeoPositions3f::GeoPropDataFieldMask);
    endEditCP(Norms, GeoNormals3f::GeoPropDataFieldMask);
    endEditCP(Indices, GeoIndicesUI32::GeoPropDataFieldMask);
    endEditCP(Tex, GeoTexCoords2f::GeoPropDataFieldMask);
    endEditCP(Mat);

    GeometryPtr geo = Geometry::create();
    beginEditCP(geo);
        geo->setTypes(Type);
        geo->setLengths(Length);
        geo->setIndices(Indices);
        geo->setPositions(Pos);
        geo->setNormals(Norms);
        geo->setTexCoords(Tex);
        if (wire) geo->setMaterial(getWiredMaterial());
        else geo->setMaterial(Mat);
    endEditCP(geo);

    return geo;
}*/

/*NodeMTRecPtr patch::makeTrianglePlane(int N, bool wire = false) {
    NodePtr n = Node::create();
    GeometryPtr geo = makeTrianglePlaneGeo(N, wire);
    beginEditCP(n);
        n->setCore(geo);
    endEditCP(n);
    return n;
}*/

void patch::calcBezQuadPlane(bezPolygon<4>& q) {
    //schrittweite
    float step = 1./(q.N-1);

    //hilfspunkte ausen und innen
    Vec3f ha[8];
    Vec3f hi[4];
    //kanten
    Vec3f r[4];

    r[0] = q.p[1]-q.p[0];
    r[1] = q.p[3]-q.p[2];
    r[2] = q.p[2]-q.p[0];
    r[3] = q.p[3]-q.p[1];

    //float rL[4];
    //for (int i=0;i<4;i++) rL[i] = r[i].length();

    if (r[0] != Vec3f(0,0,0)) {
        ha[0] = q.p[0]+projectInPlane(r[0]*(1./3.),q.n[0],false);
        ha[1] = q.p[1]+projectInPlane(-r[0]*(1./3.),q.n[1],false);
    } else {
        ha[0] = q.p[0];
        ha[1] = q.p[1];
    }

    if (r[1] != Vec3f(0,0,0)) {
        ha[2] = q.p[2]+projectInPlane(r[1]*(1./3.),q.n[2],false);
        ha[3] = q.p[3]+projectInPlane(-r[1]*(1./3.),q.n[3],false);
    } else {
        ha[2] = q.p[2];
        ha[3] = q.p[3];
    }

    if (r[2] != Vec3f(0,0,0)) {
        ha[4] = q.p[0]+projectInPlane(r[2]*(1./3.),q.n[0],false);
        ha[5] = q.p[2]+projectInPlane(-r[2]*(1./3.),q.n[2],false);
    } else {
        ha[4] = q.p[0];
        ha[5] = q.p[2];
    }

    if (r[3] != Vec3f(0,0,0)) {
        ha[6] = q.p[1]+projectInPlane(r[3]*(1./3.),q.n[1],false);
        ha[7] = q.p[3]+projectInPlane(-r[3]*(1./3.),q.n[3],false);
    } else {
        ha[6] = q.p[1];
        ha[7] = q.p[3];
    }

    //---V1
    Vec3f temp[2];
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
    Vec3f DELs[3];
    Vec3f DELt[4][3];

    //hilfskoefficienten

    vector<Vec3f> T[4];
	for (int i = 0; i < 4; i++) T[i].resize(q.N);
    Vec3f G[4][4];
    Vec3f P[4][4];
    Vec3f bs[3];

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

/*void patch::calcBezTrianglePlane(bezPolygon<3>& q) {
    //schrittweite
    float step = 1./(q.N-1);

    //normalize normals, just to be safe
    for (int i=0;i<3;i++) q.n[i].normalize();

    //Punkte Matrix
    Vec3f P[4][4][4];

    //kanten
    Vec3f r[3];
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
        if (q.n[i] == Vec3f(0,0,0)) {
            cout << "\n\nError! No normal vector!";
            return;
        }
    }

    //ecken
    P[3][0][0] = q.p[0];
    P[0][3][0] = q.p[1];
    P[0][0][3] = q.p[2];

    //Hilfspunkte auf kanten
    P[2][1][0] = P[3][0][0]+projectInPlane(r[0]*(1./3.),q.n[0],false);
    P[1][2][0] = P[0][3][0]+projectInPlane(-r[0]*(1./3.),q.n[1],false);

    P[2][0][1] = P[3][0][0]+projectInPlane(-r[1]*(1./3.),q.n[0],false);
    P[1][0][2] = P[0][0][3]+projectInPlane(r[1]*(1./3.),q.n[2],false);

    P[0][2][1] = P[0][3][0]+projectInPlane(r[2]*(1./3.),q.n[1],false);
    P[0][1][2] = P[0][0][3]+projectInPlane(-r[2]*(1./3.),q.n[2],false);

    //berechne die distanzen zwischen den hilfspunkten
    Vec3f hr[3];
    hr[0] = P[1][2][0]-P[2][1][0];
    hr[1] = P[0][1][2]-P[0][2][1];
    hr[2] = P[2][0][1]-P[1][0][2];

    //berechne die mittleren normalvectoren
    Vec3f n[3];
    n[0] = reflectInPlane(q.n[0]+q.n[1], r[0]);
    n[1] = reflectInPlane(q.n[1]+q.n[2], r[2]);
    n[2] = reflectInPlane(q.n[2]+q.n[0], r[1]);

    for (int l=0;l<3;l++) n[l] = hr[l].cross(n[l].cross(hr[l]));
    for (int l=0;l<3;l++) n[l].normalize();

    //berechne den schnittpunkt der 3 ebenen
    Matrix A = Matrix(  n[0][0], n[0][1], n[0][2], 0,
                        n[1][0], n[1][1], n[1][2], 0,
                        n[2][0], n[2][1], n[2][2], 0,
                        0,       0,       0,       1);
    Vec4f b = Vec4f(n[0].dot(P[2][1][0]), n[1].dot(P[0][2][1]), n[2].dot(P[1][0][2]), 0);
    Matrix Ai;

    if (A.det() < 0.0001 && A.det() > -0.0001) {
        for (int l=0;l<3;l++) {
            Ai = A;
            Ai[l] = b;
            P[1][1][1][l] = Ai.det()/A.det();
        }
    } else {
        //cout << "\n det = 0";
        float h = -r[0].dot(r[1])/r[1].dot(r[1]);
        P[1][1][1] = (P[1][2][0] + P[1][0][2]
                    + h*(P[0][1][2]*2-P[0][2][1]-P[0][0][3])
                    + (1-h)*(P[0][2][1]*2-P[0][3][0]-P[0][1][2]))*(1./2.);
    }

    //forward diff
    q.geo = makeTrianglePlaneGeo(q.N-1,q.wired);

    Vec3f DELu[3];
    Vec3f Bu[4];
    Vec3f DELv[3][3];
    Vec3f Bv[4][4];
    Vec3f T[3][q.N];

    GeoPositions3fPtr Pos = GeoPositions3fPtr::dcast(q.geo->getPositions());

    //Bv03 * v³ + Bv02 * v² + Bv01 * v + Bv00 = Bu0
    Bv[0][0] = P[0][0][3];
    Bv[0][1] = (P[0][1][2] - P[0][0][3])*3;
    Bv[0][2] = -P[0][1][2]*6 + (P[0][2][1]  + P[0][0][3])*3;
    Bv[0][3] = P[0][3][0] - P[0][0][3] + (P[0][1][2] - P[0][2][1])*3 ;

    //u * [Bv13 * v³ + Bv12 * v² + Bv11 * v + Bv10] = Bu1
    Bv[1][0] = (P[1][0][2] - P[0][0][3])*3;
    Bv[1][1] = (P[0][0][3] - P[1][0][2] + P[1][1][1] - P[0][1][2])*6;
    Bv[1][2] = (P[1][2][0] + P[1][0][2] - P[0][2][1] - P[0][0][3])*3 + (P[0][1][2] - P[1][1][1])*6;

    //u² * [Bv23 * v³ + Bv22 * v² + Bv21 * v + Bv20] = Bu2
    Bv[2][0] = (P[2][0][1] + P[0][0][3])*3 - P[1][0][2]*6;
    Bv[2][1] = (P[2][1][0] - P[2][0][1] + P[0][1][2] - P[0][0][3])*3 + (P[1][0][2] - P[1][1][1])*6;

    //u³ * [Bv33 * v³ + Bv32 * v² + Bv31 * v + Bv30] = Bu3
    Bv[3][0] = P[3][0][0] - P[0][0][3] + (P[1][0][2] - P[2][0][1])*3;

    //berechne schritte
    DELv[0][2] = Bv[0][3]*6*step*step*step;
    DELv[0][1] = Bv[0][2]*2*step*step + DELv[0][2];
    DELv[0][0] = Bv[0][1]*step + Bv[0][2]*step*step + DELv[0][2]*(1./6.);

    DELv[1][2] = Vec3f(0,0,0);
    DELv[1][1] = Bv[1][2]*2*step*step;
    DELv[1][0] = Bv[1][1]*step + Bv[1][2]*step*step;

    DELv[2][2] = Vec3f(0,0,0);
    DELv[2][1] = Vec3f(0,0,0);
    DELv[2][0] = Bv[2][1]*step;

    T[0][0] = Bv[0][0];
    T[1][0] = Bv[1][0];
    T[2][0] = Bv[2][0];

    int iter = 0;
    for (int i=0;i<q.N;i++) {
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
        Pos->setValue(Bu[0], iter);
        iter++;
        for (int j=1;j<q.N-i;j++) {
            Pos->setValue(Pos->getValue(iter-1)+DELu[0], iter);
            iter++;
            DELu[0] += DELu[1];
            DELu[1] += DELu[2];
        }
    }

    //calculate the normals------------------------------------------------------------
    GeoNormals3fPtr Norms = GeoNormals3fPtr::dcast(q.geo->getNormals());

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
    Vec3f Bw[3];
    Vec3f DELw[2];
    Vec3f Tw;
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
            Vec3f temp = Norms->getValue(iter-1)+DELu[0];
            Norms->setValue(temp, iter);
            DELu[0] += DELu[1];

            iter++;
        }
    }

    //calc texture coords---------------------------------------------------------
    //nicht getestet
    GeoTexCoords2fPtr Texs = GeoTexCoords2fPtr::dcast(q.geo->getTexCoords());

    //Baustelle
    Vec2f r1 = (q.tex[1]-q.tex[2])*step;
    Vec2f r2 = (q.tex[0]-q.tex[2])*step;

    iter=0;
    for (int i=0;i<q.N;i++) {
        for (int j=0;j<q.N-i;j++) {
            Vec2f temp = q.tex[2] + r1*i + r2*j;
            Texs->setValue(temp, iter);
            iter++;
        }
    }

}*/

//iteriert über die flächen der geometrie und macht bezierflächen hin
/*NodePtr patch::applyBezierOnGeometry(GeometryPtr geo, int N) {
    NodePtr n = Node::create();
    beginEditCP(n);
        //n->addChild(drawFaceNormals(geo,0.1));//
        n->setCore(Group::create());
        for (TriangleIterator it = geo->beginTriangles(); it != geo->endTriangles(); ++it) {
            bezPolygon<3> q;
            q.N = N;
            q.p[0] = it.getPosition(0);
            q.p[1] = it.getPosition(1);
            q.p[2] = it.getPosition(2);
            q.n[0] = it.getNormal(0);
            q.n[1] = it.getNormal(1);
            q.n[2] = it.getNormal(2);
            q.tex[0] = it.getTexCoords(0);
            q.tex[1] = it.getTexCoords(1);
            q.tex[2] = it.getTexCoords(2);

            calcBezTrianglePlane(q);

            beginEditCP(q.geo);
                MaterialPtr mat = geo->getMaterial();
                if (mat != NullFC) q.geo->setMaterial(mat);
            endEditCP(q.geo);

            NodePtr geoN = Node::create();
            beginEditCP(geoN);
                geoN->setCore(q.geo);
            endEditCP(geoN);
            n->addChild(geoN);
        }
    endEditCP(n);

    return n;
}*/


OSG_END_NAMESPACE;
