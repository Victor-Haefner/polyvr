#include "VRGearSegmentation.h"
#include "core/utils/toString.h"
#include "core/math/PCA.h"
#include "core/math/VRSineFit.h"
#include "core/math/partitioning/boundingbox.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRPrimitive.h"

using namespace OSG;

namespace OSG {

struct VertexRing {
    double radius;
    vector<PolarVertex> vertices;
};

struct VertexPlane {
    double position;
    vector<PolarVertex> vertices;
    vector<VertexRing> rings;
    vector<Vec2d> contour; // resampled and in polar coords
    double profMin = 1e6;
    double profMax = -1e6;

    vector<SineFit::Fit> sineFits;
    //double sineFitFreq = 0;

    void computeProfileMinMax() {
        for (auto& v : vertices) {
            double x = v.profileCoords[1];
            profMin = min(profMin, x);
            profMax = max(profMax, x);
        }
    }
};

}

VRGearSegmentation::VRGearSegmentation() {}
VRGearSegmentation::~VRGearSegmentation() {}

VRGearSegmentationPtr VRGearSegmentation::create() { return VRGearSegmentationPtr(new VRGearSegmentation()); }

void VRGearSegmentation::computeAxis() {
    PCA pca;
    pca.addMesh(obj);
    Pose res = pca.computeRotationAxis();

    axis = res.dir();
    r1 = res.x();
    r2 = res.up();
}


bool VRGearSegmentation::same(double x, double y, double eps) { return (abs(x-y) < eps); } // TODO: multiply eps with object scale

void VRGearSegmentation::computePolarVertices() {
	auto geos = obj->getChildren(true, "Geometry", true); // gear vertices

	Boundingbox bb;

	vector<Vec3d> pos;
	for (auto g : geos) {
        auto geo = dynamic_pointer_cast<VRGeometry>(g);
        Matrix4d M = geo->getMatrixTo(obj);
        M.invert();
		auto positions = geo->getMesh()->geo->getPositions();
		for (unsigned int i=0; i<positions->size(); i++) {
            Pnt3d p = Pnt3d(positions->getValue<Pnt3f>(i));
            M.mult(p, p);
            pos.push_back(Vec3d(p));
            bb.update(Vec3d(p));
        }
		//geo->makeUnique(); // TODO: what for??
    }

    PolarCoords coords(axis, pos[0]);
    coords.dir0 = r2;
    coords.dir1 = r1;

    axisOffset = bb.center(); // offset to rotation axis
    axisOffset -= axis * axisOffset.dot(axis);

	for (auto p : pos) {
		PolarVertex v(p-axisOffset);
		v.computeAndSetAttributes(coords);
		gearVertices.push_back(v);
    }
}

void VRGearSegmentation::computePlanes() {
    auto sortCB = [](const PolarVertex& a, const PolarVertex& b) -> bool {
        return a.profileCoords[0] < b.profileCoords[0];
    };

    sort(gearVertices.begin(), gearVertices.end(), sortCB);

    planes.push_back( VertexPlane() );

    int N = gearVertices.size() - 1;
    cout << "computePlanes, N verts: " << N << ", axis: " << axis << endl;
    for (int i=0; i<N; i++) {
        int x = planes.size()-1;
        gearVertices[i].setPlaneIndex(x);
        planes[x].vertices.push_back( gearVertices[i] );
        //planes[x].position = gearVertices[i].profileCoords[0];

        bool b1 = same(gearVertices[i].profileCoords[0], gearVertices[i+1].profileCoords[0], planeEps);
        if (!b1) cout << " vert coords: " << gearVertices[i].profileCoords << ", next: " << gearVertices[i+1].profileCoords << ", next same? " << b1 << endl;
        if (!b1) planes.push_back( VertexPlane() );
    }
    int x = planes.size()-1;
    gearVertices[N].setPlaneIndex(x);
    planes[x].vertices.push_back( gearVertices[N] );

    for (auto& plane : planes) {
        int N = plane.vertices.size();
        double p = 0;
        for (auto& v : plane.vertices) p += v.profileCoords[0];
        plane.position = p/N;
    }
}

void VRGearSegmentation::groupPlanes() {
    if (planes.size() < 2) return;

    for (auto& plane : planes) plane.computeProfileMinMax();

    int size = planes.size();
    for (int i=0; i<size; i++) {
        for (int j=i+1; j<size; j++) {
            if (same(planes[i].profMax, planes[j].profMax, matchEps)) matchedPlanes[i] = j;
        }
    }
}

void VRGearSegmentation::computeRings() {
    auto sortCB = [](const PolarVertex& a, const PolarVertex& b) -> bool {
        return a.radius < b.radius;
    };

	for (auto& plane : planes) {
        sort(plane.vertices.begin(), plane.vertices.end(), sortCB);

        plane.rings.push_back(VertexRing());

		int N = plane.vertices.size()-1;
		for (int i=0; i<N; i++) {
            int x = plane.rings.size()-1;
            PolarVertex& v1 = plane.vertices[i];
            PolarVertex& v2 = plane.vertices[i+1];
            plane.rings[x].vertices.push_back(v1);
            plane.rings[x].radius = v1.radius;
			bool b1 = same(v1.radius, v2.radius, ringEps);
			if (!b1) plane.rings.push_back(VertexRing());
        }
        int x = plane.rings.size()-1;
        PolarVertex& v = plane.vertices[N];
        plane.rings[x].vertices.push_back(v);
        plane.rings[x].radius = v.radius;
    }
}

void VRGearSegmentation::computeContours() {
    auto sortCB = [](const PolarVertex& a, const PolarVertex& b) -> bool {
        return a.polarCoords[0] < b.polarCoords[0];
    };

	for (auto& plane : planes) {
        double R0 = plane.rings[0].radius;
        vector<PolarVertex> outerVertices;
        auto vLast = plane.vertices[0];
        for (auto& v : plane.vertices) {
            if (plane.rings.size() > 2) {
                if (same(v.radius, R0, ringEps)) continue; // ignore inner radius
            }
            //if (same(v.polarCoords[0], vLast.polarCoords[0], 1e-3)) continue; // ignore same angles
            outerVertices.push_back(v);
            vLast = v;
        }
        sort(outerVertices.begin(), outerVertices.end(), sortCB); // sort by angles

		//for (auto v : outerVertices) plane.contour.push_back(Vec2d(v.polarCoords[0], v.polarCoords[1]));

		double di = -3.2;
		double d = 0.01;
		for (unsigned int i=1; i<outerVertices.size(); i++) {
            double a1 = outerVertices[i-1].polarCoords[0];
            double r1 = outerVertices[i-1].polarCoords[1];
            double a2 = outerVertices[i].polarCoords[0];
            double r2 = outerVertices[i].polarCoords[1];
			while (di < a1) di += d;
			if (di > a2) continue;
			while (di < a2) {
				double Px = r1+(r2-r1)/(a2-a1)*(di-a1);
				plane.contour.push_back(Vec2d(di,Px));
				di += d;
            }
        }
    }
}

void VRGearSegmentation::computeSineApprox() {
    //Fit sin to the input time sequence, and store fitting parameters "amp", "omega", "phase", "offset", "freq", "period" and "fitfunc")
    for (VertexPlane& plane : planes) {
        if (plane.contour.size() < 3) continue;

        cout << "do fft for plane " << plane.position << endl;

        SineFit sineFit;
        sineFit.vertices = plane.contour;
        sineFit.compute(fftFreqHint);
        plane.sineFits = sineFit.fits;
    }
}

void VRGearSegmentation::setBinSizes(double pe, double me, double re) {
    planeEps = pe;
    matchEps = me;
    ringEps = re;
}

void VRGearSegmentation::setFFTFreqHint(int h, int s) { fftFreqHint = h; fftFreqSel = s; }

void VRGearSegmentation::computeGearParams(int fN) {
    if (planes.size() == 1) {
        matchedPlanes[0] = 0;
    }

	for (auto& match : matchedPlanes) {
        VertexPlane& plane1 = planes[match.first];
        VertexPlane& plane2 = planes[match.second];

        if (plane1.sineFits.size() <= size_t(fN)) continue;
        if (plane2.sineFits.size() <= size_t(fN)) continue;
        if (plane1.sineFits[fN].fit.size() == 0) continue;
        if (plane2.sineFits[fN].fit.size() == 0) continue;

        auto& fit1 = plane1.sineFits[fN];
        //auto& fit2 = plane2.sineFits[fN];
        //cout << "matching planes " << Vec2f(plane1.position, plane2.position) << ", fits: " << Vec2f(fit1.quality, fit2.quality) << endl;

        double rmin = plane1.rings[0].radius;
        double rmax = plane1.rings[ plane1.rings.size()-1 ].radius;

		double r1 = rmax - 2*abs(fit1.fit[0]);
		double ts = rmax - r1;
		double R = rmax - ts*0.5;
		double f = fit1.fit[1];
		//double pitch = R*sin(1.0/f)*4;
		//double Nteeth = round(2*pi*R/pitch);
		double Nteeth = round(f);
		double pitch = 2*Pi*R/Nteeth;
		double width = abs(plane2.position - plane1.position);
		double offset = (plane1.position+plane2.position)*0.5;

        //cout << " possible gear " << Vec4d(f, pitch, Nteeth, R) << endl;
		if (Nteeth < 4) continue;

        gears.push_back( {rmin, r1, R, ts, pitch, Nteeth, width, offset} );
        //cout << "- - - computeGearParams " << Vec4d(f, pitch, Nteeth, R) << endl;
    }
}

void VRGearSegmentation::analyse(VRObjectPtr o) {
    if (!o) return;
    obj = o;
    cout << "VRGearSegmentation::analyse " << o->getName() << endl;
    cout << "  computeAxis" << endl;
    computeAxis();          // main orientation of gear, also its rotation axis
    cout << "  computePolarVertices" << endl;
    computePolarVertices(); // compute gear vertices which include the polar coordinates
    cout << "  computePlanes" << endl;
    computePlanes();        // compute the different planes the gear is made out of
    cout << "  groupPlanes" << endl;
    groupPlanes();          // match similar planes to each other
    cout << "  computeRings" << endl;
    computeRings();         // separate the verices of each plane in rings, vertices with similar radius
    cout << "  computeContours" << endl;
    computeContours();      // use outer rings to compute the contour of the gear, also resamples the contour
    cout << "  computeSineApprox" << endl;
    computeSineApprox();    // compute a sine approximation of the contours
    cout << "  computeGearParams" << endl;
    computeGearParams(fftFreqSel);
    cout << "   analysis done" << endl;
}

Vec3d VRGearSegmentation::getAxis() { return axis; }
Vec3d VRGearSegmentation::getAxisOffset() { return axisOffset; }
PosePtr VRGearSegmentation::getPolarCoords() { return Pose::create(Vec3d(0,0,0), axis, r1); }
size_t VRGearSegmentation::getNGears() { return gears.size(); }
size_t VRGearSegmentation::getNPlanes() { return planes.size(); }

vector<double> VRGearSegmentation::getGearParams(size_t i) { return i<gears.size() ? gears[i] : vector<double>(); }

VRGeometryPtr VRGearSegmentation::createGear(size_t i) {
    auto p = getGearParams(i);
    if (p.size() < 8) return 0;
    auto g = VRGeometry::create("gear");
    g->setPrimitive("Gear "+toString(p[6])+" "+toString(p[0])+" "+toString(p[4])+" "+toString(p[5])+" "+toString(p[3])+" 0"); // width hole pitch N_teeth teeth_size bevel

    g->setTransform(axis*p[7]+axisOffset, axis, r1);
    return g;
}

double VRGearSegmentation::getPlanePosition(size_t i) { return planes[i].position; }
vector<Vec2d> VRGearSegmentation::getPlaneContour(size_t i) { return planes[i].contour; }

vector<double> VRGearSegmentation::getPlaneFrequencies(size_t i) {
    vector<double> res;
    for (auto sf : planes[i].sineFits) res.push_back(sf.fit[1]);
    return res;
}

vector<double> VRGearSegmentation::getPlaneSineGuess(size_t i, size_t sf) {
    if (planes[i].sineFits.size() <= sf) return vector<double>();
    return planes[i].sineFits[sf].guess;
}

vector<double> VRGearSegmentation::getPlaneSineApprox(size_t i, size_t sf) {
    if (planes[i].sineFits.size() <= sf) return vector<double>();
    return planes[i].sineFits[sf].fit;
}

vector<Vec2d> VRGearSegmentation::getPlaneVertices(size_t i) {
    vector<Vec2d> vp;
    for (auto& v : planes[i].vertices) vp.push_back(v.polarCoords);
    return vp;
}

VRTransformPtr VRGearSegmentation::getSineFitViz(int precision) {
    auto m = VRMaterial::create("cMat");
    m->setLineWidth(2);
    m->setDiffuse(Color3f(0,0,0));
    m->setLit(0);

    auto res = VRTransform::create("gearSineFits");
	auto pc = getPolarCoords();
    size_t Np = getNPlanes();
    for (int i=0; i<Np; i++) {
        auto s = getPlaneSineApprox(i);
        if (s.size() != 4) continue;
        cout << "sineApprox " << i << " " << toString(s) << endl;
        auto p = getPlanePosition(i);
        VRGeoData curve;
        int j = precision;
        for (int i=0; i<precision; i++) {
            float a = i*2*Pi/(precision-1);
            float r = s[0] * sin(s[1]*a + s[2]) + s[3];
            float x = r*cos(a);
            float y = r*sin(a);
            Vec3d v = p*axis + axisOffset + pc->transform(Vec3d(x,y,0), false);
            curve.pushVert(v);
            curve.pushLine(i, j%precision);
            j = i;
        }

        auto geo = curve.asGeometry("curve"+toString(i));
        geo->setMaterial(m);
        res->addChild(geo);
    }

    return res;
}

VRTransformPtr VRGearSegmentation::getContourViz() {
    auto m = VRMaterial::create("cMat");
    m->setPointSize(3);
    m->setLit(0);

    auto res = VRTransform::create("gearContours");
	auto pc = getPolarCoords();
    size_t Np = getNPlanes();
    for (int i=0; i<Np; i++) {
        auto p = getPlanePosition(i);
        auto cs = getPlaneContour(i);

        VRGeoData contour;
        for (auto c : cs) {
            float x = c[1]*cos(c[0]);
            float y = c[1]*sin(c[0]);
            Vec3d R = pc->transform(Vec3d(-y, x, 0), false);
            Vec3d v = axisOffset + axis*p + R;
            Color3f col(1, 0.3*(i%3), 1-0.3*(i%3));
            contour.pushVert(v, Vec3d(0,1,0), col);
            contour.pushPoint();
        }

        auto geo = contour.asGeometry("contour"+toString(i));
        geo->setMaterial(m);
        res->addChild(geo);
    }

    return res;
}

void VRGearSegmentation::printResults() {
    if (!obj) return;
    cout << "Gear segmentation - " << obj->getName() << ", N gear vertices: " << gearVertices.size() << endl;
    cout << " axis: " << axis << ", r1: " << r1 << ", r2: " << r2 << endl;
    cout << " N planes: " << planes.size() << ", isMultiGear: " << isMultiGear << endl;
    for (auto p : planes) {
        cout << "  plane at " << p.position << ", min/max: " << Vec2d(p.profMin, p.profMax) << ", N vertices: " << p.vertices.size() << ", N rings: " << p.rings.size() << ", N contour: " << p.contour.size() << endl;

        for (auto sf : p.sineFits) {
            cout << "   guess: " << sf.guess[0] << " sin( " << sf.guess[1] << " x + " << sf.guess[2] << " ) + " << sf.guess[3] << endl;
            cout << "   sine: " << sf.fit[0] << " sin( " << sf.fit[1] << " x + " << sf.fit[2] << " ) + " << sf.fit[3] << endl;
        }

        for (auto r : p.rings) {
            cout << "   ring at " << r.radius << ", N vertices: " << r.vertices.size() << endl;
        }
    }
    cout << matchedPlanes.size() << " matched planes: " << endl;
    for (auto m : matchedPlanes) cout << "  " << m.first << ":" << m.second << endl;
    cout << " N gears: " << gears.size() << endl;
    for (auto g : gears) {
        cout << "  gear params: ";
        for (auto v : g) cout << v << " ";
        cout << endl;
    }
}

void VRGearSegmentation::runTest() {
    cout << "Start VRGearSegmentation test" << endl;
    auto g = VRGeometry::create("testGeo");
    g->setPrimitive("Gear 0.6 0.01 0.05 12 0.02 0"); // width hole pitch N_teeth teeth_size bevel
    //g->setDir(Vec3d(1,0,0));
    //g->applyTransformation();
    analyse(g);
    printResults();

    auto p = gears[0];

    cout << "Test results:" << endl;
    cout << " hole: " << p[0] << " (0.01)" << endl;
    cout << " pitch: " << p[4] << " (0.05)" << endl;
    cout << " Nteeth: " << p[5] << " (12)" << endl;
    cout << " Lteeth: " << p[3] << " (0.02)" << endl;


    //               rmin,    r1,      R,       ts,         pitch,   Nteeth, width, offset
    // gear params: -0.105092 0.104932 0.105012 0.000159588 0.010046 66      0.6    0
}
