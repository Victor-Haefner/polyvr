#include "VRKabschAlgorithm.h"

#ifndef WITHOUT_LAPACKE_BLAS
#include "core/math/SingularValueDecomposition.h"
#endif

#include <OpenSG/OSGQuaternion.h>

using namespace OSG;

VRKabschAlgorithm::VRKabschAlgorithm() {}
VRKabschAlgorithm::~VRKabschAlgorithm() {}

VRKabschAlgorithmPtr VRKabschAlgorithm::create() { return VRKabschAlgorithmPtr( new VRKabschAlgorithm() ); }
VRKabschAlgorithmPtr VRKabschAlgorithm::ptr() { return static_pointer_cast<VRKabschAlgorithm>(shared_from_this()); }

void VRKabschAlgorithm::setPoints1( vector<Vec3d>& pnts ) { points1 = pnts; }
void VRKabschAlgorithm::setPoints2( vector<Vec3d>& pnts ) { points2 = pnts; }
void VRKabschAlgorithm::setMatches( vector<Vec2i>& mths ) { matches = mths; }

void VRKabschAlgorithm::setSimpleMatches() {
    matches.clear();
    for (unsigned int i=0; i<points1.size(); i++) {
        matches.push_back(Vec2i(i,i));
    }
}

Vec3d VRKabschAlgorithm::centroid(vector<Vec3d> pnts) {
    Vec3d r;
    for (auto p : pnts) r += p;
    if (pnts.size() > 0) r /= pnts.size();
    return r;
}

Matrix4d VRKabschAlgorithm::compute(bool verbose) {
    Vec3d c1 = centroid(points1);
    Vec3d c2 = centroid(points2);

    Matrix4d H, D, U, Ut, V, Vt, T;
    H.setScale(Vec3d(0,0,0));

    // covariance matrix H
    for (auto& m : matches) {
        Vec3d p1 = points1[m[0]] - c1;
        Vec3d p2 = points2[m[1]] - c2;
        for (int i=0; i<3; i++) {
            for (int j=0; j<3; j++) {
                H[i][j] += p1[i]*p2[j];
            }
        }
    }

#ifndef WITHOUT_LAPACKE_BLAS
    SingularValueDecomposition svd(H, verbose);

    U = svd.U;
    V = svd.V;
    Ut.transposeFrom(svd.U);
    Vt.transposeFrom(svd.V);

    D = U;
    D.mult(Vt);
    float d = D.det(); // det( V Ut ) / det( U Vt )

    // R = V diag(1,1,d) Ut / R = U diag(1,1,d) Vt
    T.setScale(Vec3d(1,1,d));
    //T.setScale(Vec3d(d,d,d));
    if (verbose) {
        cout << "T\n" << T << endl;
        cout << "H\n" << H << endl;
        cout << "V\n" << V << endl;
        cout << "S\n" << svd.S << endl;
        cout << "U\n" << svd.U << endl;
        cout << "Ut\n" << Ut << endl;
        Matrix4d C = svd.check();
        cout << "check\n" << C;
        float f = 0; for (int i=0; i<4; i++) for (int j=0; j<4; j++) f += H[i][j] - C[i][j];
        cout << " -> " << f << endl << endl;
    }
    T.multLeft(U);
    T.mult(Vt);
#endif

    // compute translation

    /*vector<Vec3d> pntsT; // looks nice but is wrong :(
    for (auto p : points1) { T.mult(p,p); pntsT.push_back( p ); }
    auto cT = centroid(pntsT);
    T.setTranslate(c2-cT);*/

    Vec3d P;
    T.mult(points1[0], P);
    T.setTranslate(points2[0]-P);

    if (verbose) {
        double f;
        Vec3d Rt, Rs, Rc, ax;
        Quaterniond Rr, Rso;
        T.getTransform(Rt,Rr,Rs,Rso);
        Rr.getValueAsAxisRad(ax,f);
        cout << " Rt: " << Rt << "\n Rr: " << ax << "  " << f << endl;
        cout << " R:\n" << T << endl;
    }
    return T;
}

void VRKabschAlgorithm::test() {
    VRKabschAlgorithm a;
    vector<Vec3d> p1, p2;

    Pnt3d t(1,2,3);
    //Pnt3d t(0,0,0);
    Quaterniond r(Vec3d(1,0,1), -0.35);

    Matrix4d M;
    M.setTranslate(t);
    M.setRotate(r);

    //p1 = vector<Vec3d>( { Vec3d(1,0,0), Vec3d(1,2,0), Vec3d(1,0,3), Vec3d(4,0,-2) } );
    //p1 = vector<Vec3d>( { Vec3d(1,0,0), Vec3d(-1,0,0), Vec3d(0,0,1), Vec3d(0,0,-1) } );
    p1 = vector<Vec3d>( { Vec3d(0,0,1), Vec3d(0,0,-1), Vec3d(1,1,1), Vec3d(-1,-1,-1), Vec3d(2,2,2) } );
    for (auto v : p1) {
        Pnt3d p(v);
        M.mult(p,p);
        p2.push_back(Vec3d(p));
    }

    a.setPoints1( p1 );
    a.setPoints2( p2 );
    a.setSimpleMatches();
    auto R = a.compute();

    Vec3d ax; double f;
    r.getValueAsAxisRad(ax,f);
    cout << "\nKabschAlgorithm::test\nM:\n" << M << "\n t: " << t << "\n r: " << ax << "  " << f << endl;
    cout << " R\n" << R << endl;

    Vec3d Rt, Rs, Rc;
    Quaterniond Rr, Rso;
    R.getTransform(Rt,Rr,Rs,Rso);
    Rr.getValueAsAxisRad(ax,f);
    cout << " Rt: " << Rt << "\n Rr: " << ax << "  " << f << endl;

    for (unsigned int i=0; i<p1.size(); i++) {
        Pnt3d p;
        R.mult(Pnt3d(p1[i]),p);
        cout << " D " << Vec3d(p-p2[i]).length() << " PP " << p2[i] << " / " << p << endl;
    }
}
