#include "VRBRepSurface.h"
#include "core/math/triangulator.h"
#include "core/math/pose.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/utils/isNan.h"
#include "core/utils/toString.h"
#include "addons/Algorithms/VRMeshSubdivision.h"
#include <OpenSG/OSGTriangleIterator.h>

using namespace OSG;

static const double pi = 3.1415926535;

VRBRepSurface::VRBRepSurface() {}
VRBRepSurface::~VRBRepSurface() {}

VRBRepSurfacePtr VRBRepSurface::create() { return VRBRepSurfacePtr(new VRBRepSurface()); }

void VRBRepSurface::setPose(PosePtr pose) { trans = pose; }
void VRBRepSurface::setPlane() { stype = "Plane"; }
void VRBRepSurface::setCylinder(double radius) { stype = "Cylindrical_Surface"; R = radius; }

void VRBRepSurface::addBound(VRBRepBoundPtr bound) { bounds.push_back(bound); }

struct triangle {
    vector<Pnt3f> p; // vertex positions
    vector<Vec3f> v; // edge vectors
    vector<Vec2f> tcs; // uvs
    Vec3i i; // vertex indices
    double A = 0; // area
    Vec3f C;   // circumcenter
    double R;  // circumradius
    double R2; // circumradius square

    triangle(TriangleIterator it, bool computeCircumsphere = false, bool getTCs = false) : p(3), v(3) {
        i = Vec3i(it.getPositionIndex(0), it.getPositionIndex(1), it.getPositionIndex(2));
        for (int j=0; j<3; j++) p[j] = it.getPosition(j);
        v[0] = p[2]-p[1]; v[1] = p[2]-p[0]; v[2] = p[1]-p[0];
        A = v[0].cross(v[1]).squareLength();

        if (computeCircumsphere) {
            C = (Vec3f(p[0])+Vec3f(p[1])+Vec3f(p[2]))*1.0/3.0;
            R = C.dist(p[0]);
            R2 = R*R;
        }

        if (getTCs) {
            tcs.resize(3);
            for (int i=0; i<3; i++) tcs[i] = it.getTexCoords(i);
        }
    }

    Vec3d computeBaryCoords(Vec3f P, Vec3f n) {
        auto det = [&](Vec3f& A, Vec3f& B) {
            return A.cross(B).dot(n);
        };

        double d01 = det(v[2], v[1]);

        Vec3f p0(p[0]);
        double a =  (det(P, v[1]) - det(p0, v[1])) / d01;
        double b = -(det(P, v[2]) - det(p0, v[2])) / d01;
        double c = 1.0 - a - b;

        return Vec3d(a,b,c);
    }

    Vec2d computeBaryUV(Vec3d Pd) {
        Vec3f P = Vec3f(Pd)-Vec3f(p[0]);
        Vec3f n = v[2].cross(v[1]); n.normalize();
        P -= n * P.dot(n); // project on triangle plane
        P += Vec3f(p[0]);

        Vec2d uv;
        Vec3d abc = computeBaryCoords(P, n);
        if (tcs.size() == 3) uv = Vec2d( tcs[0]*abc[2] + tcs[1]*abc[0] + tcs[2]*abc[1] );

        //cout << "  computeBaryUV, P: " << Pd << ", P0: " << p[0] << ", P1: " << p[1] << ", P2: " << p[2] << ", abc: " << abc << endl;

        return uv;
    }

    double distToLine(Vec3f P, Vec3f A, Vec3f B) {
        Vec3f D = B-A;
        D.normalize();
        Vec3f V = P-A;
        float t = V.dot(D);
        Vec3f Pl = A+t*D;
        return Pl.dist(P);
    }

    Vec3f projected(Vec3d Pd) {
        Vec3f P = Vec3f(Pd)-Vec3f(p[0]);
        Vec3f n = v[2].cross(v[1]); n.normalize();
        P -= n * P.dot(n); // project on triangle plane
        return Vec3f(p[0])+P;
    }

    double isInside(Vec3d Pd, Vec2d& uv) {
        Vec3f P = Vec3f(Pd);
        if (P.dist2(C) > R2*1.1) return 1e6; // big distance

        P -= Vec3f(p[0]);
        Vec3f n = v[2].cross(v[1]); n.normalize();
        P -= n * P.dot(n); // project on triangle plane

        Vec3d abc = computeBaryCoords(Vec3f(p[0])+P, n);

        if (tcs.size() == 3) uv = Vec2d( tcs[0]*abc[2] + tcs[1]*abc[0] + tcs[2]*abc[1] );

        if (abc[0] >= 0 && abc[1] >= 0 && abc[2] >= 0) {
            //cout << "  tri inside, P: " << Pd << ", P0: " << p[0] << ", P1: " << p[1] << ", P2: " << p[2] << ", abc: " << abc << endl;
            return 0;
        } else {
            //return 1e6;
            if (abc[0] < 0 && abc[1] < 0) return P.length();   // distance to p0
            if (abc[0] < 0 && abc[2] < 0) return v[1].dist(P); // distance to p2
            if (abc[1] < 0 && abc[2] < 0) return v[2].dist(P); // distance to p1
            //return 1e6;
            if (abc[0] < 0) return distToLine(P, Vec3f(), v[1]);
            if (abc[1] < 0) return distToLine(P, Vec3f(), v[2]);
            if (abc[2] < 0) return distToLine(P, v[1]   , v[2]);
            return 0;
        }

        return 1e6; // big distance
    }
};

VRGeometryPtr VRBRepSurface::build(bool flat) {
    //cout << "VRSTEP::Surface build " << stype << ", outside? " << same_sense << endl;

    if (!trans) trans = Pose::create();
    Matrix4d m = trans->asMatrix();
    Vec3d d = trans->dir();
    Vec3d u = trans->up();
    Matrix4d mI = m;
    mI.invert();

    auto checkOrder = [&](Pnt3d p0, Pnt3d p1, Pnt3d p2, Vec3d n) {
        float cp = (p1-p0).cross(p2-p0).dot(n);
        return (cp >= 0);
    };

    auto pushTri = [&](VRGeoData& g, Pnt3d p1, Pnt3d p2, Pnt3d p3, Vec3d n) {
        //cout << " pushTri " << p1 << "   " << p2 << "   " << p3 << endl;
        int a = g.pushVert(p1,n);
        int b = g.pushVert(p2,n);
        int c = g.pushVert(p3,n);
        if (checkOrder(p1,p2,p3,n)) g.pushTri(a,b,c);
        else g.pushTri(a,c,b);
    };

    auto sphericalUnproject = [&](Vec3d& p, double& lastTheta, double& lastPhi, int type, double cDir = 0, bool curveEnd = false) {
        mI.mult(Pnt3d(p),p);
        Vec3d n = p*1.0/R;
        //cout << " sphericalUnproject, p: " << p << ", lastTheta: " << lastTheta << ", lastPhi: " << lastPhi << ", type: " << type << ", cDir: " << cDir << ", curveEnd: " << curveEnd << endl;
        //cout << " -> R: " << R << ", n: " << n << " nL: " << n.length() << endl;
        if (n[2] <= -1.0) n[2] = -1.0;
        if (n[2] >=  1.0) n[2] =  1.0;
        double theta = asin(n[2]); // theta, angle to up axis z, -pi/2 -> pi/2
        double phi   = atan2(n[1], n[0]); // phi, angle in horizontal plane, -pi -> pi

        // ambigous points, when theta points up or down, phi can be any value-> take old one!
        if (abs(theta) > pi*0.5-1e-3) phi = lastPhi;

        if (abs(phi) > pi-1e-3) { // ambigous point on +- pi
            //cout << " !!! amb point?: " << phi << ", cDir: " << cDir << ", type: " << type << ", curveEnd: " << curveEnd << endl;
            if (type == 1) { // circle or B-Spline
                if (!curveEnd) phi = lastPhi;
                if (cDir > 0 && curveEnd) phi =  pi;
                if (cDir < 0 && curveEnd) phi = -pi;
                //cout << "   set circle phi: " << phi << endl;
            }

            if (type == 2) { // B-Spline, stay close to old value!
                if (!curveEnd) phi = lastPhi;
                if (lastPhi < 0 && curveEnd) phi = -pi;
                if (lastPhi > 0 && curveEnd) phi =  pi;
                //cout << "   set spline phi: " << phi << endl;
            }
        }

        //cout << "   -> theta, phi: " << theta << ", " << phi << endl;

        lastTheta = theta;
        lastPhi = phi;
        return Vec2d(theta, phi);
    };

    auto toroidalUnproject = [&](Vec3d& p, double& lastTheta, double& lastPhi, int type, double cDir = 0, bool curveEnd = false) {
        mI.mult(Pnt3d(p),p);
        Vec3d nR = p;
        nR[2] = 0;
        nR.normalize();
        double phi = atan2(nR[1], nR[0]); // phi, angle in horizontal plane, -pi -> pi

        Vec3d pR = nR * R; // middle of ring
        Vec3d pr = p-pR;
        Vec3d n = pr*1.0/R2;

        if (n[2] <= -1.0) n[2] = -1.0;
        if (n[2] >=  1.0) n[2] =  1.0;
        //cout << "   n: " << n << "   ";
        double theta = asin(n[2]); // theta, angle around torus, -pi -> pi
        if (n.dot(nR) < 0 && theta <  0) theta = -pi-theta;
        if (n.dot(nR) < 0 && theta >= 0) theta =  pi-theta;

        // ambigous points, theta on pi line, can be + or - pi
        if (abs(theta) > pi-1e-3) {
            if (lastTheta > 0) theta =  pi;
            if (lastTheta < 0) theta = -pi;
        }

        if (abs(phi) > pi-1e-3) { // ambigous point on +- pi
            //cout << "  !F=pi, lF=" << lastPhi;
            //cout << " !!! amb point?: " << phi << ", cDir: " << cDir << ", type: " << type << ", curveEnd: " << curveEnd << endl;
            /*if (type == 1) { // circle
                if (!curveEnd) phi = lastPhi;
                if (cDir > 0 && curveEnd) phi =  pi;
                if (cDir < 0 && curveEnd) phi = -pi;
                cout << "   set circle phi: " << phi << endl;
            }

            if (type == 2) { // B-Spline, stay close to old value!
                if (!curveEnd) phi = lastPhi;
                if (lastPhi < 0 && curveEnd) phi = -pi;
                if (lastPhi > 0 && curveEnd) phi =  pi;
                //cout << "   set spline phi: " << phi << endl;
            }*/
            if (lastPhi < 0) phi = -pi;
            if (lastPhi > 0) phi =  pi;
            //cout << "->" << phi << "  ";
        }

        //cout << "   -> theta, phi: " << theta << ", " << phi << endl;

        lastTheta = theta;
        lastPhi = phi;
        return Vec2d(theta, phi);
    };

    auto cylindricUnproject = [&](Matrix4d& mI, Vec3d p, double& lastAngle, int type, double cDir = 0, bool circleEnd = false) {
        mI.mult(Pnt3d(p),p);
        //cout << " cylindricUnproject, p: " << p << ", lastAngle: " << lastAngle << ", type: " << type << ", cDir: " << cDir << ", circleEnd: " << circleEnd;
        double h = p[2];
        double a = atan2(p[1]/R, p[0]/R);

        bool validLastAngle = bool(lastAngle < 999);

        if (abs(a) > pi-1e-3) { // ambigous point on +- pi
            //cout << "  amb point?: " << a << ", cDir: " << cDir << endl;
            if (type == 0 && validLastAngle) a = lastAngle; // next point on line

            if (type == 1) { // circle
                if (!circleEnd) a = lastAngle;
                else a = cDir*pi;
                //cout << "   set circle a: " << a << endl;
            }

            if (type == 2) { // B-Spline, stay close to old value!
                if (!circleEnd) a = lastAngle;
                else {
                    if (lastAngle < 0) a = -pi;
                    if (lastAngle > 0) a =  pi;
                }
                //cout << "   set spline a: " << a << endl;
            }
        }

        if (validLastAngle && abs(cDir) > 0.5) { // a circle point
            double da = a - lastAngle;
            if (cDir*da < 0) {  // check if the circle turn in good direction
                //cout << "Warning! wrong circle turning direction! la->a: " << lastAngle << "->" << a << " (cDir:" << cDir << ")";
                a += cDir*2*pi;
                //cout << " ... la->a: " << lastAngle << "->" << a << endl;
            }
        }

        //cout << " -> (a,h): " << Vec2d(a,h) << endl;

        lastAngle = a;
        return Vec2d(a,h);
    };

    auto conicUnproject = [&](Vec3d& p, double& lastAngle, int type, double cDir = 0, bool circleEnd = false) {
        mI.mult(Pnt3d(p),p);
        //cout << " conicUnproject, p: " << p << ", lastAngle: " << lastAngle << ", type: " << type << ", cDir: " << cDir << ", circleEnd: " << circleEnd << endl;

        double h = p[2];
        //double r = R*h*tan(R2); // R2 is angle from vertical to cone surface
        double a = atan2(p[1], p[0]);
        //cout << " -> R: " << r << ", r: " << p[0]*p[0]+p[1]*p[1] << " -> a: " << a << ", h: " << h << endl;

        if (abs(a) > pi-1e-3) { // ambigous point on +- pi
            //cout << "  amb point?: " << a << ", cDir: " << cDir << endl;
            if (type == 0 && lastAngle != 1000) a = lastAngle; // next point on line

            if (type == 1) { // circle
                if (!circleEnd) a = lastAngle;
                if (cDir > 0 && circleEnd) a =  pi;
                if (cDir < 0 && circleEnd) a = -pi;
                //cout << "   set circle a: " << a << endl;
            }

            if (type == 2) { // B-Spline, stay close to old value!
                if (!circleEnd) a = lastAngle;
                if (lastAngle < 0 && circleEnd) a = -pi;
                if (lastAngle > 0 && circleEnd) a =  pi;
                //cout << "   set spline a: " << a << endl;
            }
        }

        lastAngle = a;
        return Vec2d(a,h);
    };

    auto toroidalProject = [&](const Vec2d& p, Vec3d& n) {
        double theta = p[0];
        double phi   = p[1];

        Vec3d pRing = Vec3d(cos(phi), sin(phi), 0) * R; // middle of ring
        n = Vec3d(cos(phi)*cos(theta), sin(phi)*cos(theta), sin(theta));

        Pnt3d P(pRing + n * R2);
        //cout << "    torus theta: " << theta << ", phi: " << phi << ", pos: " << P << ", R: " << R << ", R2: " << R2 << endl;
        return P;
    };

    // TODO: this may fail if inner polygons are not moved accordingly
    //  maybe it would be better to make a border instead of moving points
    auto fixPolyJump = [&](VRPolygon& poly) {
        auto& points = poly.get();
        for (int i=1; i<points.size(); i++) {
            Vec2d& p1 = points[i-1];
            Vec2d& p2 = points[i];
            if (abs(p2[0] - p1[0]) > 1.5*pi) {
                if (p2[0] < 0) p2[0] += 2*pi;
                else p2[0] -= 2*pi;
            }
        }
    };

    auto checkPolyIntegrety = [&](VRPolygon& poly) { // for cylinder
        auto& points = poly.get();
        for (int i=1; i<points.size(); i++) {
            Vec2d p1 = points[i-1];
            Vec2d p2 = points[i];
            if (abs(p2[0] - p1[0]) > 1.5*pi) {
                cout << "Warning! cylinder polygon jump detected: " << p1 << " -> " << p2 << endl;
                fixPolyJump(poly); // try to fix it
                return;
            }
        }
    };

    auto checkPolyOrientation = [&](VRPolygon& poly, VRBRepBoundPtr bound) {
        bool isCCW = poly.isCCW();
        bool isOuter = bound->isOuter();
        if (same_sense) {
            if (!isCCW && isOuter) poly.reverseOrder();
            if (isCCW && !isOuter) poly.reverseOrder();
        } else {
            if (isCCW && isOuter) poly.reverseOrder();
            if (!isCCW && !isOuter) poly.reverseOrder();
        }
    };

    auto computeSplineRes = [&](field<Vec3d>& cpoints) {
        double LcurvU = 0;
        double LcurvV = 0;
        for (int i=1; i<cpoints.width; i++) {
            Vec3d p1 = cpoints.get(i-1,0);
            Vec3d p2 = cpoints.get(i,0);
            LcurvV += (p2-p1).length();
        }
        for (int j=1; j<cpoints.height; j++) {
            Vec3d p1 = cpoints.get(0,j-1);
            Vec3d p2 = cpoints.get(0,j);
            LcurvU += (p2-p1).length();
        }

        double K = 2*pi*30;
        int resI = ceil(Ncurv*LcurvU/K);
        int resJ = ceil(Ncurv*LcurvV/K);
        resI = max(2, resI);
        resJ = max(2, resJ);
        //cout << "res: " << Vec2i(resI, resJ) << ", L: " << Vec2i(LcurvU, LcurvV) << endl;
        return Vec2i(resI, resJ);
    };

    auto wireBounds = [&](vector<VRBRepBoundPtr>& bounds) {
        VRGeoData data;

        Color3f col(1,1,0);
        if (!same_sense) col = Color3f(1,0,1);

        for (auto b : bounds) {
            auto points = b->getPoints();
            for (uint i=0; i<points.size(); i++) {
                Pnt3d p = points[i];
                mI.mult(Pnt3d(p),p);
                data.pushVert(p, Vec3d(0,1,0), col);
                if (i > 0) data.pushLine();
            }
        }

        auto geo = data.asGeometry("facePlaceHolder");
        VRMaterialPtr mat = VRMaterial::create("face");
        mat->setLit(0);
        mat->setLineWidth(4);
        geo->setMaterial(mat);
        return geo;
    };

    auto visualizeCircles = [&](vector<VRBRepBoundPtr>& bounds) {
        VRGeoData data;

        Color3f col(1,1,0);
        if (!same_sense) col = Color3f(1,0,1);

        auto addPnt = [&](Pnt3d p, Color3f c) {
            mI.mult(Pnt3d(p),p);
            data.pushVert(p, Vec3d(0,1,0), c);
        };

        auto addVec = [&](Pnt3d p1, Pnt3d p2, Color3f c1, Color3f c2) {
            addPnt(p1, c1);
            addPnt(p2, c2);
            data.pushLine();
        };

        for (auto b : bounds) {
            for (auto& e : b->getEdges()) {
                if (e->etype != "Circle") continue;

                auto c = e->center;
                auto r = e->radius;
                addVec(c->pos(), c->pos()+c->dir()*r*0.1, Color3f(0,0,1), Color3f(0,0,1));
                addVec(c->pos(), c->pos()+c->up()*r*0.1, Color3f(0,1,0), Color3f(0,1,0));
                addVec(c->pos(), c->pos()+c->x()*r*0.1, Color3f(1,0,0), Color3f(1,0,0));
                addVec(c->pos(), e->EBeg, Color3f(1,1,1), Color3f(1,0,0));
                addVec(c->pos(), e->EEnd, Color3f(1,1,1), Color3f(0,1,0));
            }
        }

        auto geo = data.asGeometry("circlesVisu");
        VRMaterialPtr mat = VRMaterial::create("circles");
        mat->setLit(0);
        mat->setLineWidth(4);
        geo->setMaterial(mat);
        return geo;
    };

    auto computeExtend = [&](VRGeometryPtr g, int dim) {
        Vec2d range(1e6, -1e6);
        if (g) if (auto gg = g->getMesh()) {
            GeoVectorPropertyMTRecPtr pos = gg->geo->getPositions();
            if (pos) {
                for (uint i=0; i<pos->size(); i++) {
                    Pnt3d p = Pnt3d(pos->getValue<Pnt3f>(i));
                    double h = p[dim];
                    if (h < range[0]) range[0] = h;
                    if (h > range[1]) range[1] = h;
                }
            }
        }
        return range[1]-range[0];
    };

    //if (stype != "Spherical_Surface") return 0;

    if (stype == "Plane") {
        //return 0;
        //cout << "make Plane, N bounds: " << bounds.size() << endl;
        Triangulator t;
        if (bounds.size() == 0) cout << "Warning: No bounds!\n";

        vector<int> sortedBounds;
        for (int i=0; i<bounds.size(); i++) if ( bounds[i]->isOuter()) sortedBounds.push_back(i);
        for (int i=0; i<bounds.size(); i++) if (!bounds[i]->isOuter()) sortedBounds.push_back(i);

        for (auto& bi : sortedBounds) {
            auto& b = bounds[bi];
            //if (b.points.size() == 0) cout << "Warning: No bound points for bound " << b.BRepType << endl;
            bool doPrint = b->containsNan();
            VRPolygon poly;

            for (auto pIn : b->getPoints()) {
                Vec3d pOut;
                if (doPrint) cout << " pIn: " << pIn << endl;
                mI.multFull(Pnt3d(pIn), pOut);
                if (doPrint) cout << " pOut: " << pOut << " " << mI[0] << ", " << mI[1] << ", " << mI[2] << ", " << mI[3] << endl;
                if (abs(pOut[2]) > 0.001) cout << " Error in VRBRepSurface::build Plane, p[2] not 0 (" << pOut[2] << ")! -> data loss" << endl;
                poly.addPoint(pOut);
            }

            checkPolyOrientation(poly, b);
            t.add(poly);

            //cout << "  add polygon: " << toString(poly.get3()) << endl;
        }

        auto g = t.compute();
        if (!g) return 0;
        if (!g->getMesh()->geo->getPositions()) cout << "NO MESH!\n";
        if (!flat) g->setMatrix(m);


        //g->addChild(wireBounds(bounds));

        Vec3d nP = Vec3d(0, 0, 1);
        if (!same_sense) nP *= -1;
        GeoVectorPropertyMTRecPtr norms = g->getMesh()->geo->getNormals();
        for (uint i=0; i<norms->size(); i++) norms->setValue(nP, i);

        return g;
    }

    if (stype == "Cylindrical_Surface") {
        //return 0;
        Triangulator triangulator; // feed the triangulator with unprojected points

        for (auto b : bounds) {
            //cout << "Cylinder bound: " << b->isOuter() << endl;
            VRPolygon poly;
            double lastAngle = 1000;
            Vec3d cN(0,0,1);

            // shift edges so first edge not start on +-pi line
            auto eOnPiLine = [&](VRBRepEdgePtr e) {
                Vec3d p = e->points[0];
                mI.mult(Pnt3d(p),p);
                if (abs(p[1]) > 1e-3) return false;
                if (p[0] > 1e-3) return false;
                return true;
            };

            auto edges = b->getEdges();
            if (eOnPiLine(edges[0]))  {
                int i0 = -1;
                for (int i=1; i<edges.size(); i++) {
                    if (!eOnPiLine(edges[i])) {
                        i0 = i;
                        break;
                    }
                }

                b->shiftEdges(i0);
            }

            for (auto& e : b->getEdges()) {
                //cout << "  bound edge: " << e->etype << endl;
                if (e->etype == "Circle") {
                    //cout << "circle edge: " << Vec2d(e->a1, e->a2) << ", ";
                    //compareCoordSystems(trans, e->center, 1e-3);

                    double cDir = e->compCircleDirection(trans, cN);

                    if (poly.size() == 0) {
                        Vec2d p1 = cylindricUnproject(mI, e->EBeg, lastAngle, 1, cDir, false);
                        poly.addPoint(p1);
                    }

                    Vec2d p2 = cylindricUnproject(mI, e->EEnd, lastAngle, 1, cDir, true);
                    poly.addPoint(p2);
                    continue;
                }

                if (e->etype == "Line") {
                    if (poly.size() == 0) {
                        Vec2d p1 = cylindricUnproject(mI, e->EBeg, lastAngle, 0);
                        poly.addPoint(p1);
                    }

                    Vec2d p2 = cylindricUnproject(mI, e->EEnd, lastAngle, 0);
                    poly.addPoint(p2);
                    continue;
                }

                if (e->etype == "B_Spline_Curve_With_Knots") {
                    int i0 = 1;
                    if (poly.size() == 0) i0 = 0;
                    for (int i=i0; i<e->points.size(); i++) {
                        auto& p = e->points[i];
                        Vec2d pc = cylindricUnproject(mI, p, lastAngle, 2, 0, i>0);
                        poly.addPoint(pc);
                    }
                    continue;
                }

                cout << "Unhandled edge on cylinder of type " << e->etype << endl;
            }

            checkPolyIntegrety(poly);
            checkPolyOrientation(poly, b);

            /*VRPolygon poly2;
            poly2.addPoint(Vec2d(-0.0900314, 105.9));
            poly2.addPoint(Vec2d(-0.0900314, 0));
            poly2.addPoint(Vec2d(2.95475, 0));
            poly2.addPoint(Vec2d(2.95475, 105.9));*/

            triangulator.add(poly, b->isOuter());
            //cout << " poly : " << toString(poly.get())  << endl;
            //cout << " poly2: " << toString(poly2.get()) << endl;
        }

        auto g = triangulator.compute();
        if (!g) return 0;
        if (auto gg = g->getMesh()->geo) { if (!gg->getPositions()) cout << "VRBRepSurface::build: Triangulation failed, no mesh positions!\n";
        } else cout << "VRBRepSurface::build: Triangulation failed, no mesh generated!\n";


        VRMeshSubdivision subdiv;
        double H = computeExtend(g, 2);
        subdiv.subdivideGrid(g, Vec3d(Dangle, -1, H), false);

        if (g && !flat) if (auto gg = g->getMesh()) {
            // project the points back into 3D space
            GeoVectorPropertyMTRecPtr pos = gg->geo->getPositions();
            GeoVectorPropertyMTRecPtr norms = gg->geo->getNormals();
            if (pos) {
                for (uint i=0; i<pos->size(); i++) {
                    Pnt3d p = Pnt3d(pos->getValue<Pnt3f>(i));
                    Vec3d n = Vec3d(norms->getValue<Vec3f>(i));
                    double a = p[0];
                    double h = p[2];
                    n = Vec3d(cos(a), sin(a), 0);

                    p[2] = h;
                    p[1] = n[1]*R;
                    p[0] = n[0]*R;

                    pos->setValue(p, i);

                    if (!same_sense) n *= -1;
                    norms->setValue(n, i);
                }
            }
        }

        //g->addChild( wireBounds(bounds) );
        //g->addChild( visualizeCircles(bounds) );
        //auto tBounds = triangulator.computeBounds();
        //g->addChild(tBounds);
        //g->getMaterial()->setFrontBackModes(GL_FILL, GL_LINE); // to test face orientations*/

        if (g && !flat) g->setMatrix(m);
        //if (g) g->setScale(Vec3d(1,1,0.001));
        if (g && g->getMesh() && g->getMesh()->geo->getPositions() && g->getMesh()->geo->getPositions()->size() > 0) return g;
        return 0;
    }

    if (stype == "B_Spline_Surface") {
        //return 0;
        cout << "B_Spline_Surface" << endl;

        // ROADMAP
        //  first idea:
        //   - tesselate whole BSpline surface (lots of quads)
        //   - keep uv map of the resulting mesh
        //   - for each edge point:
        //      - get nearest quad to point
        //      - get UV koords of the point on that quad
        //      - add point UV to polyline
        //   - triangulate in UV space
        //   - reuse first tesselation
        //      - cut quads traversed by edges
        //      - ignore quads outside of the triangulation


        Vec3d n(0,0,1);
        VRGeoData nMesh;

        map<int, map<int, int> > ids;

        cout << "B_Spline_Surface du " << degu << " dv " << degv << "  pw " << cpoints.width << " ph " << cpoints.height << endl;

        Vec2i res = computeSplineRes(cpoints);
        float T = 1; //knots[knots.size()-1] - knots[0];
        for (int i=0; i<=res[0]; i++) {
            float u = i*T/res[0];
            for (int j=0; j<=res[1]; j++) {
                float v = j*T/res[1];
                Vec3d p = BSpline(u,v, degu, degv, cpoints, knotsu, knotsv);
                ids[i][j] = nMesh.pushVert(p,n);

                if (i > 0 && j%2 == 0) nMesh.pushTri(ids[i][j], ids[i-1][j], ids[i-1][j+1]);
                if (i > 0 && j%2 == 1) nMesh.pushTri(ids[i][j], ids[i][j-1], ids[i-1][j]);
            }
        }

        auto g = nMesh.asGeometry("BSpline");


        // feed the triangulator with unprojected points
        /*Triangulator t;
        for (auto b : bounds) {
            VRPolygon poly;
            for(auto p : b.points) {
                mI.mult(Pnt3d(p),p);
                float u = p[0];
                float v = p[1];
                poly.addPoint(Vec2d(u, v));
            }
            if (!poly.isCCW()) poly.turn();
            t.add(poly);
        }
        auto g = t.compute();*/

        // tesselate the result while projecting it back on the surface
        /*if (g) if (auto gg = g->getMesh()) {
            TriangleIterator it;
            VRGeoData nMesh;
            Vec3d n(0,0,1);

            auto checkOrder = [&](Pnt3d p0, Pnt3d p1, Pnt3d p2) {
                float cp = (p1-p0).cross(p2-p0).dot(n);
                return (cp >= 0);
            };

            auto pushTri = [&](Pnt3d p1, Pnt3d p2, Pnt3d p3) {
                int a = nMesh.pushVert(p1,n);
                int b = nMesh.pushVert(p2,n);
                int c = nMesh.pushVert(p3,n);
                if (checkOrder(p1,p2,p3)) nMesh.pushTri(a,b,c);
                else nMesh.pushTri(a,c,b);
            };

            auto pushQuad = [&](Pnt3d p1, Pnt3d p2, Pnt3d p3, Pnt3d p4) {
                int a = nMesh.pushVert(p1,n);
                int b = nMesh.pushVert(p2,n);
                int c = nMesh.pushVert(p3,n);
                int d = nMesh.pushVert(p4,n);
                if (checkOrder(p1,p2,p3)) nMesh.pushTri(a,b,c);
                else nMesh.pushTri(a,c,b);
                if (checkOrder(p2,p3,p4)) nMesh.pushTri(b,c,d);
                else nMesh.pushTri(b,d,c);
            };

            for (it = TriangleIterator(gg); !it.isAtEnd() ;++it) {
                triangle t(it);
                if (t.A < 1e-6) continue; // ignore flat triangles

                Vec3i pOrder(0,1,2); // get the order of the vertices
                for (int i=0; i<3; i++) { // max 3 sort steps
                    if (pSides[pOrder[0]] > pSides[pOrder[1]]) swap(pOrder[0], pOrder[1]);
                    else if (pSides[pOrder[0]] > pSides[pOrder[2]]) swap(pOrder[0], pOrder[2]);
                    else if (pSides[pOrder[1]] > pSides[pOrder[2]]) swap(pOrder[1], pOrder[2]);
                }

                if (pSides[0] == pSides[1] && pSides[0] == pSides[2]) {
                    pushTri(t.p[0],t.p[1],t.p[2]);
                    continue;
                }

                //cout << " unhandled triangle " << endl;
            }

            nMesh.apply(g);

            // project the points back into 3D space
            GeoVectorPropertyMTRecPtr pos = gg->getPositions();
            GeoVectorPropertyMTRecPtr norms = gg->getNormals();
            if (pos) {
                for (int i=0; i<pos->size(); i++) {
                    Pnt3d p = pos->getValue<Pnt3f>(i);
                    Vec3d n = norms->getValue<Vec3f>(i);
                    n = Vec3d(cos(p[0]), sin(p[0]), 0);

                    Vec2d side = getSide(p[0]);
                    Vec3d A = Vec3d(R*cos(side[0]), R*sin(side[0]), 0);
                    Vec3d B = Vec3d(R*cos(side[1]), R*sin(side[1]), 0);
                    Vec3d D = B-A;
                    D.normalize();

                    float t = (A[0]*D[1] - A[1]*D[0]) / (n[0]*D[1] - n[1]*D[0]);

                    //cout << "p: " << p[0]/Pi*180 << " AB: " << side*(180/Pi) << " s: " << getSideN(p[0]) << endl;
                    //if (p[0] < side[0] || p[0] > side[1]) cout << "   AAAH\n"; // TODO: check this out!

                    p[2] = p[1];
                    p[1] = n[1]*t;
                    p[0] = n[0]*t;

                    pos->setValue(p, i);
                    norms->setValue(n, i);
                }
            }
        }*/

        if (g && !flat) g->setMatrix(m);
        if (g && g->getMesh() && g->getMesh()->geo->getPositions() && g->getMesh()->geo->getPositions()->size() > 0) return g;
        return 0;
    }

    if (stype == "B_Spline_Surface_With_Knots") {
        //return 0;
        //cout << "B_Spline_Surface_With_Knots" << endl;
        // ROADMAP
        //  first idea:
        //   - tesselate whole BSpline surface (lots of quads)
        //   - keep uv map of the resulting mesh
        //   - for each edge point:
        //      - get nearest quad to point
        //      - get UV koords of the point on that quad
        //      - add point UV to polyline
        //   - triangulate in UV space
        //   - reuse first tesselation
        //      - cut quads traversed by edges
        //      - ignore quads outside of the triangulation

        bool isWeighted = (weights.height == cpoints.height && weights.width == cpoints.width);

        Vec3d n(0,0,1);
        VRGeoData nMesh;

        if (knotsu.size() == 0 || knotsv.size() == 0) return 0;

        // BSpline mesh
        map<int, map<int, int> > ids;
        Vec2i res = computeSplineRes(cpoints);
        float Tu = knotsu[knotsu.size()-1] - knotsu[0];
        float Tv = knotsv[knotsv.size()-1] - knotsv[0];
        for (int i=0; i<=res[0]; i++) {
            float u = knotsu[0]+i*Tu/res[0];
            //cout << " i: " << i << " Tu: " << Tu << " u: " << u << " knotsu[0]: " << knotsu[0] << " knotsu[-1]: " << knotsu[knotsu.size()-1] << endl;
            for (int j=0; j<=res[1]; j++) {
                float v = knotsv[0]+j*Tv/res[1];
                Vec3d p = isWeighted ? BSpline(u,v, degu, degv, cpoints, knotsu, knotsv, weights) : BSpline(u,v, degu, degv, cpoints, knotsu, knotsv);
                Vec3d n = isWeighted ? BSplineNorm(u,v, degu, degv, cpoints, knotsu, knotsv, weights) : BSplineNorm(u,v, degu, degv, cpoints, knotsu, knotsv);
                if (same_sense) n *= -1;
                ids[i][j] = nMesh.pushVert(p,n,Vec2d(u,v));

                if (same_sense) {
                    if (i > 0 && j > 0) nMesh.pushQuad(ids[i][j], ids[i-1][j], ids[i-1][j-1], ids[i][j-1]);
                } else {
                    if (i > 0 && j > 0) nMesh.pushQuad(ids[i][j], ids[i][j-1], ids[i-1][j-1], ids[i-1][j]);
                }
            }
        }

        // compute bounds on surface
        auto g = nMesh.asGeometry("BSpline");
        if (g) if (auto gg = g->getMesh()) {
            Triangulator triangulator;

            for (auto b : bounds) {
                auto points = b->getPoints();
                //cout << " BSpline Bound, outer: " << b->isOuter() << " " << points.size() << endl;

                /*for (auto& e : b.edges) {
                    cout << "  edge " << e.etype << ", Np: " << e.points.size() << ", BE: " << e.EBeg << " -> " << e.EEnd << endl;
                }*/
                VRPolygon poly;

                vector<triangle> triangles;
                TriangleIterator it;
                for (it = TriangleIterator(gg->geo); !it.isAtEnd() ;++it) {
                    triangles.push_back( triangle(it, true, true ));
                }

                for (auto p : points) {
                    //if (p[0] > 85) continue; // for testing

                    mI.multFull(p, p);
                    //cout << "bound point: " << p << endl;
                    double dmin = 1e5;
                    int imin = -1;
                    for (int i=0; i<triangles.size(); i++) {
                        auto& tri = triangles[i];
                        Vec2d uv;
                        double d = tri.isInside(p, uv);
                        //cout << "  dist tri: " << d << endl;

                        if (d < dmin) {
                            dmin = d;
                            imin = i;
                        }

                        if (d < 1e-3) break; // is inside
                    }
                    //cout << " imin: " << imin << ", dmin: " << dmin << ", Ntri: " << triangles.size() << endl;

                    if (imin >= 0) {
                        auto& tri = triangles[imin];
                        Vec2d uv = tri.computeBaryUV(p);
                        poly.addPoint(uv);
                        Vec3d p2 = isWeighted ? BSpline(uv[0],uv[1], degu, degv, cpoints, knotsu, knotsv, weights) : BSpline(uv[0],uv[1], degu, degv, cpoints, knotsu, knotsv);
                        //if (p2.dist(p) > 10)
                        //if (p2.length() < 10)
                        //cout << "imin: " << imin<< ", dmin: " << dmin << endl;
                        //    cout << " bound point unprojected, p -> uv -> p: " << p << " -> " << uv << " -> " << p2 << ", D: " << p2.dist(p) << endl;
                    }
                }

                //checkPolyOrientation(poly, b);
                triangulator.add(poly);
                //cout << "  BSpline poly   points: " << toString(poly.get()) << endl;
                //cout << "  BSpline bounds points: " << toString(b->getPoints()) << endl;
            }

            g = triangulator.compute();
            VRMeshSubdivision subdiv;
            auto gres = Vec3d(Tu/res[0], -1, Tv/res[1]);
            subdiv.subdivideGrid( g, gres, false);

            if (g && !flat) if (auto gg = g->getMesh()) {
                GeoVectorPropertyMTRecPtr pos = gg->geo->getPositions();
                GeoVectorPropertyMTRecPtr norms = gg->geo->getNormals();
                if (pos) {
                    for (uint i=0; i<pos->size(); i++) {
                        Pnt3f uv = pos->getValue<Pnt3f>(i);

                        double u = uv[0];
                        double v = uv[2];
                        Vec3d p = isWeighted ? BSpline(u,v, degu, degv, cpoints, knotsu, knotsv, weights) : BSpline(u,v, degu, degv, cpoints, knotsu, knotsv);
                        Vec3d n = isWeighted ? BSplineNorm(u,v, degu, degv, cpoints, knotsu, knotsv, weights) : BSplineNorm(u,v, degu, degv, cpoints, knotsu, knotsv);
                        if (same_sense) n *= -1;

                        pos->setValue(p, i);
                        norms->setValue(n, i);
                    }
                }
            }
        }


        if (g && !flat) g->setMatrix(m);
        if (g && g->getMesh() && g->getMesh()->geo->getPositions() && g->getMesh()->geo->getPositions()->size() > 0) return g;
        return 0;
    }

    if (stype == "Conical_Surface") {
        Triangulator triangulator; // feed the triangulator with unprojected points
        h0 = -R/tan(R2);
        //cout << "Conical_Surface, R: " << R << ", R2: " << R2 << ", h0: " << h0 << endl;


        for (auto b : bounds) {
            VRPolygon poly;
            double lastAngle = 1000;
            Vec3d cN(0,0,1);

            // shift edges so first edge not start on +-pi line
            auto eOnPiLine = [&](VRBRepEdgePtr e) {
                Vec3d p = e->points[0];
                mI.mult(Pnt3d(p),p);
                if (abs(p[1]) > 1e-3) return false;
                if (p[0] > 1e-3) return false;
                return true;
            };

            auto edges = b->getEdges();
            if (eOnPiLine(edges[0]))  {
                int i0 = -1;
                for (int i=1; i<edges.size(); i++) {
                    if (!eOnPiLine(edges[i])) {
                        i0 = i;
                        break;
                    }
                }

                b->shiftEdges(i0);
            }

            for (auto& e : b->getEdges()) {
                if (e->etype == "Circle") {
                    double cDir = e->compCircleDirection(trans, cN);

                    if (poly.size() == 0) {
                        Vec2d p1 = conicUnproject(e->EBeg, lastAngle, 1, cDir, false);
                        poly.addPoint(p1);
                    }

                    Vec2d p2 = conicUnproject(e->EEnd, lastAngle, 1, cDir, true);
                    poly.addPoint(p2);
                    continue;
                }

                if (e->etype == "Line") {
                    if (poly.size() == 0) {
                        Vec2d p1 = conicUnproject(e->EBeg, lastAngle, 0);
                        poly.addPoint(p1);
                    }

                    Vec2d p2 = conicUnproject(e->EEnd, lastAngle, 0);
                    poly.addPoint(p2);
                    continue;
                }

                if (e->etype == "B_Spline_Curve_With_Knots") {
                    int i0 = 1;
                    if (poly.size() == 0) i0 = 0;
                    for (int i=i0; i<e->points.size(); i++) {
                        auto& p = e->points[i];
                        Vec2d pc = conicUnproject(p, lastAngle, 2, 0, i>0);
                        poly.addPoint(pc);
                    }
                    continue;
                }

                cout << "Unhandled edge on cone of type " << e->etype << endl;
            }

            checkPolyIntegrety(poly);
            checkPolyOrientation(poly, b);
            triangulator.add(poly);
        }

        auto g = triangulator.compute();
        if (!g) return 0;
        if (auto gg = g->getMesh()->geo) { if (!gg->getPositions()) cout << "VRBRepSurface::build: Triangulation failed, no mesh positions!\n";
        } else cout << "VRBRepSurface::build: Triangulation failed, no mesh generated!\n";

        VRMeshSubdivision subdiv;
        double H = computeExtend(g, 2);
        subdiv.subdivideGrid(g, Vec3d(Dangle*0.5, -1, H), false);


        if (g && !flat) if (auto gg = g->getMesh()) {
            // project the points back into 3D space
            GeoVectorPropertyMTRecPtr pos = gg->geo->getPositions();
            GeoVectorPropertyMTRecPtr norms = gg->geo->getNormals();
            if (pos) {
                for (uint i=0; i<pos->size(); i++) {
                    Pnt3d p = Pnt3d(pos->getValue<Pnt3f>(i));
                    double a = p[0];
                    double h = p[2];
                    Vec3d n = Vec3d(cos(a), sin(a), 0);

                    double r = (h-h0)*tan(R2); // R2 is angle from vertical to cone surface

                    p[2] = h;
                    p[1] = n[1]*r;
                    p[0] = n[0]*r;
                    //cout << " a: " << a << ", h: " << h << " -> p: " << p << endl;

                    pos->setValue(p, i);

                    double b = pi-R2;
                    if (h < h0) b = pi+R2; // TODO: this line is not tested!

                    n = Vec3d(cos(a)*cos(b), sin(a)*cos(b), sin(b));
                    if (same_sense) n *= -1;
                    norms->setValue(n, i);
                }
            }
        }

        //auto geo = wireBounds(bounds);
        //g->addChild(geo);

        if (g && !flat) g->setMatrix(m);
        if (g && g->getMesh() && g->getMesh()->geo->getPositions() && g->getMesh()->geo->getPositions()->size() > 0) return g;
        return 0;
    }

    if (stype == "Spherical_Surface") {
        //cout << "Spherical_Surface" << endl;
        Triangulator triangulator; // feed the triangulator with unprojected points

        for (auto b : bounds) {
            VRPolygon poly;
            double lastTheta = 1000;
            double lastPhi   = 1000;
            Vec3d cN = Vec3d(0,0,1);

            // shift edges so first edge not start on +-pi line
            auto eOnPiLine = [&](VRBRepEdgePtr e) {
                Vec3d p = e->points[0];
                mI.mult(Pnt3d(p),p);
                if (abs(p[1]) > 1e-3) return false;
                if (p[0] > 1e-3) return false;
                return true;
            };

            auto edges = b->getEdges();
            if (eOnPiLine(edges[0]))  {
                int i0 = -1;
                for (int i=1; i<edges.size(); i++) {
                    if (!eOnPiLine(edges[i])) {
                        i0 = i;
                        break;
                    }
                }

                b->shiftEdges(i0);
            }

            for (auto& e : b->getEdges()) {
                //cout << " edge on sphere " << e->etype << endl;
                if (e->etype == "Circle") {
                    double cDir = e->compCircleDirection(trans, cN);

                    int i0 = 1;
                    if (poly.size() == 0) i0 = 0;
                    for (int i=i0; i<e->points.size(); i++) {
                        auto& p = e->points[i];
                        Vec2d pc = sphericalUnproject(p, lastTheta, lastPhi, 1, cDir, i>0);
                        //cout << " ---- " << p << " -> " << pc << endl;
                        poly.addPoint(pc);
                    }
                    continue;
                }

                if (e->etype == "B_Spline_Curve_With_Knots") {
                    int i0 = 1;
                    if (poly.size() == 0) i0 = 0;
                    for (int i=i0; i<e->points.size(); i++) {
                        auto& p = e->points[i];
                        Vec2d pc = sphericalUnproject(p, lastTheta, lastPhi, 2, 0, i>0);
                        cout << " ---- " << p << " -> " << pc << endl;
                        poly.addPoint(pc);
                    }
                    continue;
                }

                cout << "Unhandled edge on sphere of type " << e->etype << endl;
            }

            checkPolyOrientation(poly, b);
            triangulator.add(poly);
            //cout << "  poly: " << toString(poly.get()) << endl;
        }

        auto g = triangulator.compute();
        if (!g) return 0;
        if (auto gg = g->getMesh()->geo) { if (!gg->getPositions()) cout << "VRBRepSurface::build: Triangulation failed, no mesh positions!\n";
        } else cout << "VRBRepSurface::build: Triangulation failed, no mesh generated!\n";

        VRMeshSubdivision subdiv;
        auto res = Vec3d(Dangle, -1, Dangle);
        subdiv.subdivideGrid(g, res, false);

        // tesselate the result while projecting it back on the surface
        if (g && !flat) if (auto gg = g->getMesh()) {
            // project the points back into 3D space
            GeoVectorPropertyMTRecPtr pos = gg->geo->getPositions();
            GeoVectorPropertyMTRecPtr norms = gg->geo->getNormals();
            if (pos) {
                for (uint i=0; i<pos->size(); i++) {
                    Pnt3d p = Pnt3d(pos->getValue<Pnt3f>(i));
                    Vec3d n = Vec3d(norms->getValue<Vec3f>(i));
                    double theta = p[0];
                    double phi   = p[2];
                    n = Vec3d(cos(phi)*cos(theta), sin(phi)*cos(theta), sin(theta));

                    p = Pnt3d(n*R);

                    //cout << "    sphere theta: " << theta << ", phi: " << phi << ", pos: " << p << ", R: " << R << endl;
                    if (!same_sense) n *= -1;
                    pos->setValue(p, i);
                    norms->setValue(n, i);
                }
            }
        }

        if (g && !flat) g->setMatrix(m);
        if (g && g->getMesh() && g->getMesh()->geo->getPositions() && g->getMesh()->geo->getPositions()->size() > 0) return g;
        return 0;
    }

    if (stype == "Toroidal_Surface") {
        //cout << "Toroidal_Surface R: " << R << ", r: " << R2 << ", same_sense: " << same_sense << endl;
        Triangulator triangulator; // feed the triangulator with unprojected points

        for (auto b : bounds) {
            //cout << " toroidal bound" << endl;
            VRPolygon poly;
            double lastTheta = 1000;
            double lastPhi   = 1000;
            Vec3d cN = Vec3d(0,0,1);

            // shift edges so first edge not start on +-pi line
            auto eOnPiLine = [&](VRBRepEdgePtr e) {
                Vec3d p = e->points[0];
                mI.mult(Pnt3d(p),p);
                //cout << "  on pi line? " << p << endl;
                if (abs(p[1]) < 1e-3 && p[0] < 1e-3) return true; // phi on +-pi line
                if (abs(p[2]) < 1e-3) { // theta on 0 or +-pi
                    if (p.length() < R) return true; // theta on +-pi
                }
                //cout << "   .. no!" << endl;
                return false;
            };

            auto edges = b->getEdges();
            if (eOnPiLine(edges[0])) {
                int i0 = -1;
                for (int i=1; i<edges.size(); i++) {
                    if (!eOnPiLine(edges[i])) {
                        i0 = i;
                        break;
                    }
                }

                //cout << "  shiftEdges " << i0 << endl;
                b->shiftEdges(i0);
            }

            //Vec3d n;

            for (auto& e : b->getEdges()) {
                //cout << " edge on torus " << e.etype << endl;
                if (e->etype == "Circle") {
                    double cDir = e->compCircleDirection(trans, cN);

                    int i0 = 1;
                    if (poly.size() == 0) i0 = 0;
                    for (int i=i0; i<e->points.size(); i++) {
                        auto& p = e->points[i];
                        Vec2d pc = toroidalUnproject(p, lastTheta, lastPhi, 1, cDir, i>0);
                        //auto P = toroidalProject(pc, n);
                        //if (P.dist(p) < 0.1) cout << " --- Circle: " << p << " -> " << pc << " -> " << P << endl;
                        //else cout << " !!! Circle: " << p << " -> " << pc << " -> " << P << endl;
                        poly.addPoint(pc);
                    }
                    continue;
                }

                if (e->etype == "B_Spline_Curve_With_Knots") {
                    int i0 = 1;
                    if (poly.size() == 0) i0 = 0;
                    for (int i=i0; i<e->points.size(); i++) {
                        auto& p = e->points[i];
                        Vec2d pc = toroidalUnproject(p, lastTheta, lastPhi, 2, 0, i>0);
                        //auto P = toroidalProject(pc, n);
                        //if (P.dist(p) < 0.01) cout << " --- Curve: " << p << " -> " << pc << " -> " << P << endl;
                        //else cout << " !!! Circle: " << p << " -> " << pc << " -> " << P << endl;
                        poly.addPoint(pc);
                    }
                    continue;
                }

                cout << "Unhandled edge on sphere of type " << e->etype << endl;
            }

            checkPolyOrientation(poly, b);
            triangulator.add(poly);
            //cout << "  poly: " << toString(poly.get()) << endl;
        }

        auto g = triangulator.compute();
        if (!g) return 0;
        if (auto gg = g->getMesh()->geo) { if (!gg->getPositions()) cout << "VRBRepSurface::build: Triangulation failed, no mesh positions!\n";
        } else cout << "VRBRepSurface::build: Triangulation failed, no mesh generated!\n";

        VRMeshSubdivision subdiv;
        auto res = Vec3d(Dangle, -1, Dangle);
        subdiv.subdivideGrid(g, res, false);

        // tesselate the result while projecting it back on the surface
        if (g && !flat) if (auto gg = g->getMesh()) {
            // project the points back into 3D space
            GeoVectorPropertyMTRecPtr pos = gg->geo->getPositions();
            GeoVectorPropertyMTRecPtr norms = gg->geo->getNormals();
            if (pos) {
                for (uint i=0; i<pos->size(); i++) {
                    Vec3d n;
                    Pnt3d p = Pnt3d(pos->getValue<Pnt3f>(i));
                    p = toroidalProject(Vec2d(p[0], p[2]), n);

                    if (!same_sense) n *= -1;
                    pos->setValue(p, i);
                    norms->setValue(n, i);
                }
            }
        }

        //g->addChild(wireBounds(bounds));

        if (g && !flat) g->setMatrix(m);
        if (g && g->getMesh() && g->getMesh()->geo->getPositions() && g->getMesh()->geo->getPositions()->size() > 0) return g;
        return 0;
    }

    cout << "VRBRepSurface::build Error: unhandled surface type " << stype << endl;

    // wireframe
    VRGeoData data;

    for (auto b : bounds) {
        auto points = b->getPoints();
        for (uint i=0; i<points.size(); i+=2) {
            Pnt3d p1 = points[i];
            Pnt3d p2 = points[i+1];
            data.pushVert(p1, Vec3d(0,1,0));
            data.pushVert(p2, Vec3d(0,1,0));
            data.pushLine();
        }
    }

    auto geo = data.asGeometry("facePlaceHolder");
    VRMaterialPtr mat = VRMaterial::create("face");
    mat->setLit(0);
    mat->setLineWidth(3);
    geo->setMaterial(mat);
    return geo;
}

