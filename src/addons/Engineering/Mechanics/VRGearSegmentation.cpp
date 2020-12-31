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
        Vec3d o = getOrthogonal(coords.axis);
        Vec3d o2 = o.cross(coords.axis);
        Vec3d p = vertex;

        p = p-o*o.dot(p);
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

struct VertexPlane {
    double position;
    vector<GearVertex> vertices;
    vector<VertexRing> rings;
    vector<Vec2d> contour; // resampled and in polar coords
    double profMin = 1e6;
    double profMax = 0;

    vector<double> sineFitParams; // A, w, p, c -> A sin ( w t + p ) +c
    double sineFitFreq = 0;

    void computeProfileMinMax() {
        for (auto& v : vertices) {
            profMin = min(profMin, v.profileCoords[1]);
            profMax = max(profMin, v.profileCoords[1]);
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
}


bool same(double x, double y) { return (abs(x-y) < 1); }
bool samePlane(double x, double y) { return (abs(x-y) < 1.3); }

void VRGearSegmentation::computePolarVertices() {
	auto geos = obj->getChildren(true, "Geometry", true); // gear vertices

	vector<Vec3d> pos;
	for (auto g : geos) {
        auto geo = dynamic_pointer_cast<VRGeometry>(g);
		auto positions = geo->getMesh()->geo->getPositions();
		for (unsigned int i=0; i<positions->size(); i++) {
            pos.push_back(Vec3d(positions->getValue<Pnt3f>(i)));
        }
		geo->makeUnique();
    }

    PolarCoords coords(axis, pos[0]);

	for (auto p : pos) {
		GearVertex v(p);
		v.computeAndSetAttributes(coords);
		gearVertices.push_back(v);
    }
}

void VRGearSegmentation::computePlanes() {
    auto sortCB = [](const GearVertex& a, const GearVertex& b) -> bool {
        return a.profileCoords[0] > b.profileCoords[0];
    };

    sort(gearVertices.begin(), gearVertices.end(), sortCB);

    planes.push_back( VertexPlane() );

    int N = gearVertices.size() - 1;
    for (int i=0; i<N; i++) {
        int x = planes.size()-1;
        gearVertices[i].setPlaneIndex(x);
        planes[x].vertices.push_back( gearVertices[i] );
        planes[x].position = gearVertices[i].profileCoords[0];

        bool b1 = same(gearVertices[i].profileCoords[0], gearVertices[i+1].profileCoords[0]);
        if (!b1) planes.push_back( VertexPlane() );
    }
    int x = planes.size()-1;
    gearVertices[N].setPlaneIndex(x);
    planes[x].vertices.push_back( gearVertices[N] );
}

void VRGearSegmentation::groupPlanes() {
    if (planes.size() < 2) return;

    for (auto& plane : planes) plane.computeProfileMinMax();

    int size = planes.size();
    for (int i=0; i<size; i++) {
        for (int j=i+1; j<size; j++) {
            if (same(planes[i].profMax, planes[j].profMax)) matchedPlanes[i] = j;
        }
    }
}

void VRGearSegmentation::computeRings() {
    auto sortCB = [](const GearVertex& a, const GearVertex& b) -> bool {
        return a.radius > b.radius;
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
			bool b1 = same(v1.radius, v2.radius);
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
        return a.polarCoords[0] > b.polarCoords[0];
    };

	for (auto& plane : planes) {
        double R0 = plane.rings[0].radius;
        vector<GearVertex> outerVertices;
        for (auto& v : plane.vertices) {
            if (same(v.radius, R0)) continue; // ignore inner radius
            outerVertices.push_back(v);
        }
        sort(outerVertices.begin(), outerVertices.end(), sortCB); // sort by angles

		double di = -3.2;
		double d = 0.01;
		for (unsigned int i=0; i<outerVertices.size()-1; i++) {
            double a1 = outerVertices[i].polarCoords[0];
            double r1 = outerVertices[i].polarCoords[1];
            double a2 = outerVertices[i+1].polarCoords[0];
            double r2 = outerVertices[i+1].polarCoords[1];
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

int getArgMax(vector<double> Fyy, int offset) {
    int r = 0;
    double m = -1e6;
    for (unsigned int i=offset; i<Fyy.size(); i++) {
        if (Fyy[i] > m) {
            m = Fyy[i];
            r = i;
        }
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

    // jacoby matrices, contains the partial derivatives
    int df(const VectorXd &p, MatrixXd &jac) const {
        for (int i = 0; i < values(); i++) {
            jac(i,0) = -sin(p[1]*X[i] + p[2]);
            jac(i,1) = -cos(p[1]*X[i] + p[2])*X[i];
            jac(i,2) = -cos(p[1]*X[i] + p[2]);
            jac(i,3) = -1;
        }
        return 0;
    }
};

vector<double> sine_fit(vector<double> X, vector<double> Y, vector<double> initial_guess) {
    // the following starting values provide a rough fit.
    VectorXd x = Map<VectorXd>(&initial_guess[0], initial_guess.size());

    // do the computation
    FunctorSine functor(X,Y);
    LevenbergMarquardt<FunctorSine> lm(functor);
    lm.minimize(x);

    vector<double> res( x.data(), x.data() + 4 );
    return res;
}

const double pi = 3.14159;

void VRGearSegmentation::computeSineApprox() {
    //Fit sin to the input time sequence, and store fitting parameters "amp", "omega", "phase", "offset", "freq", "period" and "fitfunc")

    for (auto& plane : planes) {
        vector<double> tt, yy;

        for (auto& p : plane.contour) {
            tt.push_back(p[0]+pi);
            yy.push_back(p[1]);
        }

		double guess_offset = calcMean(yy);
		double guess_amp = calcMeanDeviation(yy, guess_offset) * sqrt(2);

        FFT fft;
        auto ff = fft.freq(tt.size(), tt[1]-tt[0]);
        auto Fyy = fft.transform(yy, yy.size());

        int FyyMaxPos = getArgMax(Fyy, 1); // excluding the zero frequency "peak", which is related to offset

		double guess_freq = abs(ff[FyyMaxPos+1]);

		vector<double> guess = {guess_amp, 2*pi*guess_freq, 0, guess_offset};
		//function< double(double, const vector<double>&) > sinfunc = [](double t, const vector<double>& p) -> double { return p[0] * sin(p[1]*t + p[2]) + p[3]; }; // t, A, w, p, c
        plane.sineFitParams = sine_fit(tt, yy, guess);
		plane.sineFitFreq = plane.sineFitParams[1]/(2*pi);
		//return {"amp": A, "omega": w, "phase": p, "offset": c, "freq": f, "period": 1./f, "fitfunc": fitfunc, "maxcov": numpy.max(pcov), "rawres": (guess,popt,pcov)}
        //print( "Amplitude=%(amp)s, Angular freq.=%(omega)s, phase=%(phase)s, offset=%(offset)s, Max. Cov.=%(maxcov)s" % res )
    }
}

void VRGearSegmentation::computeGearParams() {
	for (auto& match : matchedPlanes) {
        VertexPlane& plane1 = planes[match.first];
        VertexPlane& plane2 = planes[match.second];

        double rmin = plane1.profMin;
        double rmax = plane1.profMax;

		double r1 = rmax - 2*abs(plane1.sineFitParams[0]);
		double ts = rmax - r1;
		double R = rmax - ts*0.5;
		double f = plane1.sineFitParams[1];
		double pitch = R*sin(1.0/f);
		double Nteeth = round(2*pi*R/pitch);
		double width = abs(plane2.position - plane1.position);
		double offset = (plane1.position+plane2.position)*0.5;

        gears.push_back( {rmin, r1, R, ts, pitch, Nteeth, width, offset} );
    }
}

void VRGearSegmentation::analyse(VRObjectPtr o) {
    obj = o;

    computeAxis();          // main orientation of gear, also its rotation axis
    computePolarVertices(); // compute gear vertices which include the polar coordinates
    computePlanes();        // compute the different planes the gear is made out of
    groupPlanes();          // match similar planes to each other
    computeRings();         // separate the verices of each plane in rings, vertices with similar radius
    computeContours();      // use outer rings to compute the contour of the gear, also resamples the contour
    computeSineApprox();    // compute a sine approximation of the contours
    computeGearParams();
}

Vec3d VRGearSegmentation::getAxis() { return axis; }
int VRGearSegmentation::getNGears() { return gears.size(); }

vector<double> VRGearSegmentation::getPlanePositions() {
    vector<double> res;
    for (auto& p : planes) res.push_back(p.position);
    return res;
}

vector<double> VRGearSegmentation::getGearParams(int i) {
    return gears[i];
}

