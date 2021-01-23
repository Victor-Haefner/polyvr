#include "VRGearSegmentation.h"
#include "core/utils/toString.h"
#include "core/math/PCA.h"
#include "core/math/fft.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"

#include <unsupported/Eigen/NonLinearOptimization>

using namespace OSG;
using namespace Eigen;

template<> string typeName(const VRGearSegmentation& m) { return "GearSegmentation"; }

namespace OSG {
struct PolarCoords {
    Vec3d axis;
    Vec3d dir0;
    Vec3d dir1;

    PolarCoords(Vec3d a, Vec3d p0) {
        axis = a;
        dir0 = p0 - a*a.dot(p0);
        dir0.normalize();
        dir1 = dir0.cross(a);
        dir1.normalize();
    }
};

struct GearVertex {
    Vec3d vertex;
	Vec2d polarCoords;
	Vec2d profileCoords;
	int plane = -1;
	double radius = -1;

	GearVertex() {}
	GearVertex(Vec3d v) : vertex(v) {}

	Vec3d getOrthogonal(Vec3d v) {
		if (abs(v[0]-v[1]) < 0.1) return Vec3d(v[0],-v[2],v[1]);
		return Vec3d(v[1],-v[0],v[2]);
    }

    double computeRadius(PolarCoords& coords) {
        Vec3d proj = vertex - coords.axis*coords.axis.dot(vertex);
        return proj.length();
    }

    Vec2d computePolar(PolarCoords& coords) {
        if (radius < 0) computeRadius(coords);

        Vec3d p = vertex;
        p.normalize();

        double angle = atan2(p.dot(coords.dir0), p.dot(coords.dir1));
        return Vec2d(angle, radius);
    }

	Vec2d computeProfile(PolarCoords& coords) {
        //Vec3d o = getOrthogonal(coords.axis);
        //Vec3d o2 = o.cross(coords.axis);
        Vec3d p = vertex;
        Vec3d o = coords.dir0;
        Vec3d o2 = coords.dir1;

        p = p - o*o.dot(p);
        return Vec2d(coords.axis.dot(p), o2.dot(p));
    }

	void computeAndSetAttributes(PolarCoords& coords) {
        profileCoords = computeProfile(coords); //TODO investigate why this needs to be run before computePolar()
        radius = computeRadius(coords);
        polarCoords = computePolar(coords);
	}

	void setPlaneIndex(int i) { plane = i; }
};

struct VertexRing {
    double radius;
    vector<GearVertex> vertices;
};

struct sineFit {
    vector<double> guess;
    vector<double> sineFitParams; // A, w, p, c -> A sin ( w t + p ) +c
};

struct VertexPlane {
    double position;
    vector<GearVertex> vertices;
    vector<VertexRing> rings;
    vector<Vec2d> contour; // resampled and in polar coords
    double profMin = 1e6;
    double profMax = -1e6;

    vector<sineFit> sineFits;
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
    Pose res = pca.compute();

	double a = res.scale()[0];
	double b = res.scale()[1];
	double c = res.scale()[2];

	double AB = abs(a-b);
	double AC = abs(a-c);
	double BC = abs(b-c);

    axis = res.up();
    r1 = res.x();
    r2 = res.dir();

	if (AC < AB && AC < BC) {
        axis = res.x();
        r1 = res.up();
        r2 = res.dir();
    }

	if (BC < AB && BC < AC)  {
        axis = res.dir();
        r1 = res.x();
        r2 = res.up();
    }

	axis.normalize();

	cout << "VRGearSegmentation::computeAxis 1: " << res.scale() << "; " << res.dir() << "; " << res.x() << "; " << res.up() << endl;
	cout << "VRGearSegmentation::computeAxis 2: " << axis << "; " << r1 << "; " << r2 << endl;
}


bool VRGearSegmentation::same(double x, double y, double eps) { return (abs(x-y) < eps); } // TODO: multiply eps with object scale

void VRGearSegmentation::computePolarVertices() {
	auto geos = obj->getChildren(true, "Geometry", true); // gear vertices

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
        }
		geo->makeUnique();
    }

    PolarCoords coords(axis, pos[0]);
    coords.dir0 = r2;
    coords.dir1 = r1;

	for (auto p : pos) {
		GearVertex v(p);
		v.computeAndSetAttributes(coords);
		gearVertices.push_back(v);
    }
}

void VRGearSegmentation::computePlanes() {
    auto sortCB = [](const GearVertex& a, const GearVertex& b) -> bool {
        return a.profileCoords[0] < b.profileCoords[0];
    };

    sort(gearVertices.begin(), gearVertices.end(), sortCB);

    planes.push_back( VertexPlane() );

    int N = gearVertices.size() - 1;
    for (int i=0; i<N; i++) {
        int x = planes.size()-1;
        gearVertices[i].setPlaneIndex(x);
        planes[x].vertices.push_back( gearVertices[i] );
        //planes[x].position = gearVertices[i].profileCoords[0];

        bool b1 = same(gearVertices[i].profileCoords[0], gearVertices[i+1].profileCoords[0], planeEps);
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
    auto sortCB = [](const GearVertex& a, const GearVertex& b) -> bool {
        return a.radius < b.radius;
    };

	for (auto& plane : planes) {
        sort(plane.vertices.begin(), plane.vertices.end(), sortCB);

        plane.rings.push_back(VertexRing());

		int N = plane.vertices.size()-1;
		for (int i=0; i<N; i++) {
            int x = plane.rings.size()-1;
            GearVertex& v1 = plane.vertices[i];
            GearVertex& v2 = plane.vertices[i+1];
            plane.rings[x].vertices.push_back(v1);
            plane.rings[x].radius = v1.radius;
			bool b1 = same(v1.radius, v2.radius, ringEps);
			if (!b1) plane.rings.push_back(VertexRing());
        }
        int x = plane.rings.size()-1;
        GearVertex& v = plane.vertices[N];
        plane.rings[x].vertices.push_back(v);
        plane.rings[x].radius = v.radius;
    }
}

void VRGearSegmentation::computeContours() {
    auto sortCB = [](const GearVertex& a, const GearVertex& b) -> bool {
        return a.polarCoords[0] < b.polarCoords[0];
    };

	for (auto& plane : planes) {
        double R0 = plane.rings[0].radius;
        vector<GearVertex> outerVertices;
        auto vLast = plane.vertices[0];
        for (auto& v : plane.vertices) {
            if (same(v.radius, R0, ringEps)) continue; // ignore inner radius
            //if (same(v.polarCoords[0], vLast.polarCoords[0], 1e-3)) continue; // ignore same angles
            outerVertices.push_back(v);
            vLast = v;
        }
        sort(outerVertices.begin(), outerVertices.end(), sortCB); // sort by angles

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

double calcMean(vector<double>& f) {
    double m = 0;
    for (auto& v : f) m += v;
    m /= f.size();
    return m;
}

double calcMeanDeviation(vector<double>& f, double mean) {
    double S = 0;
    for (auto v : f) {
        S += abs(v-mean);
    }
    return S / f.size();
}

vector<int> getArgMax(vector<double> Fyy, int offset, int N) {
    vector<int> r(N,0);
    vector<double> m(N,-1e6);
    for (unsigned int i=offset; i<Fyy.size(); i++) {
        for (int j=0; j<N; j++) {
            if (Fyy[i] > m[j]) {
                m[j] = Fyy[i];
                r[j] = i;
                break;
            }
        }
    }

    cout << "FFT results:";
    for (auto i=0; i<N; i++) {
        cout << " (" << r[i] << "," << m[i] << ")";
    }

    return r;
}

struct FunctorSine {
    typedef Eigen::Matrix<double,Dynamic,1> InputType;
    typedef Eigen::Matrix<double,Dynamic,1> ValueType;
    typedef Eigen::Matrix<double,Dynamic,Dynamic> JacobianType;

    vector<double> X;
    vector<double> Y;
    int m_inputs = 4;
    int m_values = Dynamic;

    FunctorSine(vector<double> x, vector<double> y) : X(x), Y(y),  m_values(X.size()) {}

    int inputs() const { return m_inputs; }
    int values() const { return m_values; }

    // the sine function
    int operator() (const VectorXd &p, VectorXd &f) const {
        for (int i = 0; i < values(); i++) f[i] = Y[i] - (p[0] * sin(p[1]*X[i] + p[2]) + p[3]);
        return 0;
    }

    /*
    a*x + d             -> a
    a sin(b x + c) + d  -> a cos(b x + c) b
    a sin(x + c) + d    -> a cos(b x + c)
    a*x + d             -> a
    */

    // jacoby matrices, contains the partial derivatives
    int df(const VectorXd &p, MatrixXd &jac) const {
        for (int i = 0; i < values(); i++) {
            jac(i,0) = -sin(p[1]*X[i] + p[2]);
            jac(i,1) = -p[0] * cos(p[1]*X[i] + p[2])*X[i];
            jac(i,2) = -p[0] * cos(p[1]*X[i] + p[2]);
            jac(i,3) = -1;
        }
        return 0;
    }
};

const double pi = 3.14159;

void VRGearSegmentation::computeSineApprox() {
    //Fit sin to the input time sequence, and store fitting parameters "amp", "omega", "phase", "offset", "freq", "period" and "fitfunc")
    for (auto& plane : planes) {
        if (plane.contour.size() < 3) continue;

        vector<double> X, Y;
        for (auto& p : plane.contour) {
            X.push_back(p[0]+pi);
            Y.push_back(p[1]);
        }

		double guess_offset = calcMean(Y);
		double guess_amp = calcMeanDeviation(Y, guess_offset) * sqrt(2);

        //TODO: check the fft, the result is pretty bad..
        double Df = X[X.size()-1] - X[0];
        Df = 2*pi*(2*pi/Df);
        //double Df = 2*pi;

        FFT fft;
        vector<double> ff = fft.freq(X.size(), X[1]-X[0]);
        vector<double> Fyy = fft.transform(Y, Y.size());


        vector<int> freqs = getArgMax(Fyy, 1, fftFreqHint); // excluding the zero frequency "peak", which is related to offset
        for (auto FyyMaxPos : freqs) {
            double guess_freq = abs(ff[FyyMaxPos]);

            cout << "fft freq params: (" << X.size() << ", " << X[1]-X[0] << ")"<< endl;
            cout << "fft results, peak at: " << FyyMaxPos << ", " << ff[FyyMaxPos] << ", Df: " << Vec2d(2*pi, Df) << endl;

            sineFit sf;
            sf.guess = {guess_amp, Df*guess_freq, 0, guess_offset}; //
            VectorXd x = Map<VectorXd>(&sf.guess[0], sf.guess.size());

            // do the computation
            //if (fftFreqHint == plane.sineFits.size()) {
                FunctorSine functor(X,Y);
                LevenbergMarquardt<FunctorSine> lm(functor);
                lm.parameters.factor = 100; // 100
                lm.parameters.maxfev = 100*(x.size()+1); // 400
                lm.parameters.ftol = 1e-6; // eps
                lm.parameters.xtol = 1e-6; // eps
                lm.parameters.gtol = 0; // 0
                lm.parameters.epsfcn = 0; // 0
                int info = lm.minimize(x);
                //int info = lm.lmder1(x,1e-5);

                cout << "VRGearSegmentation::computeSineApprox minimize info: " << info << ", itrs: " << Vec2i(lm.nfev, lm.njev) << ", norm: " << Vec2d(lm.fnorm, lm.gnorm) << endl;

                sf.sineFitParams = vector<double>( x.data(), x.data() + 4 );
                //sf.sineFitParams[1] *= 0.25;
                //sf.sineFitFreq = sf.sineFitParams[1]/(2*pi);
            //}

            plane.sineFits.push_back(sf);
        }
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
        if (plane1.sineFits[fN].sineFitParams.size() == 0) continue;
        if (plane2.sineFits[fN].sineFitParams.size() == 0) continue;

        double rmin = plane1.rings[0].radius;
        double rmax = plane1.rings[ plane1.rings.size()-1 ].radius;

		double r1 = rmax - 2*abs(plane1.sineFits[fN].sineFitParams[0]);
		double ts = rmax - r1;
		double R = rmax - ts*0.5;
		double f = plane1.sineFits[fN].sineFitParams[1];
		//double pitch = R*sin(1.0/f)*4;
		//double Nteeth = round(2*pi*R/pitch);
		double Nteeth = round(f);
		double pitch = 2*pi*R/Nteeth;
		double width = abs(plane2.position - plane1.position);
		double offset = (plane1.position+plane2.position)*0.5;

        gears.push_back( {rmin, r1, R, ts, pitch, Nteeth, width, offset} );
        cout << "- - - computeGearParams " << Vec4d(f, pitch, Nteeth, R) << endl;
    }
}

void VRGearSegmentation::analyse(VRObjectPtr o) {
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
PosePtr VRGearSegmentation::getPolarCoords() { return Pose::create(Vec3d(0,0,0), axis, r1); }
size_t VRGearSegmentation::getNGears() { return gears.size(); }
size_t VRGearSegmentation::getNPlanes() { return planes.size(); }

vector<double> VRGearSegmentation::getGearParams(size_t i) { return i<gears.size() ? gears[i] : vector<double>(); }

VRGeometryPtr VRGearSegmentation::createGear(size_t i) {
    auto p = getGearParams(i);
    if (p.size() < 8) return 0;
    auto g = VRGeometry::create("gear");
    g->setPrimitive("Gear "+toString(p[6])+" "+toString(p[0])+" "+toString(p[4])+" "+toString(p[5])+" "+toString(p[3])+" 0"); // width hole pitch N_teeth teeth_size bevel

    g->setTransform(axis*p[7], axis, r1);
    return g;
}

double VRGearSegmentation::getPlanePosition(size_t i) { return planes[i].position; }
vector<Vec2d> VRGearSegmentation::getPlaneContour(size_t i) { return planes[i].contour; }

vector<double> VRGearSegmentation::getPlaneSineGuess(size_t i, size_t sf) {
    if (planes[i].sineFits.size() <= sf) return vector<double>();
    return planes[i].sineFits[sf].guess;
}

vector<double> VRGearSegmentation::getPlaneSineApprox(size_t i, size_t sf) {
    if (planes[i].sineFits.size() <= sf) return vector<double>();
    return planes[i].sineFits[sf].sineFitParams;
}

vector<Vec2d> VRGearSegmentation::getPlaneVertices(size_t i) {
    vector<Vec2d> vp;
    for (auto& v : planes[i].vertices) vp.push_back(v.polarCoords);
    return vp;
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
            cout << "   sine: " << sf.sineFitParams[0] << " sin( " << sf.sineFitParams[1] << " x + " << sf.sineFitParams[2] << " ) + " << sf.sineFitParams[3] << endl;
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
