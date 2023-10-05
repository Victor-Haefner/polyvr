#include "VRThreadSegmentation.h"
#include "core/utils/toString.h"
#include "core/math/PCA.h"
#include "core/math/VRSineFit.h"
#include "core/math/partitioning/boundingbox.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeoData.h"

/**

Helix formula:
  H(t) = [ R cos(t), R sin(t), k t ]
   -> t = x/k
   -> H(x) = [ R cos(x/k), R sin(x/k), x ]
  project in profile plane
   -> P(x) = R sin(x/k+a)

Strategy:
- PCA -> main/rotation axis
- compute max radius
- filter points close to that radius
- project points in profile plane
- visualize it..

*/

using namespace OSG;

VRThreadSegmentation::VRThreadSegmentation() {}
VRThreadSegmentation::~VRThreadSegmentation() {}

VRThreadSegmentationPtr VRThreadSegmentation::create() { return VRThreadSegmentationPtr(new VRThreadSegmentation()); }

void VRThreadSegmentation::computeAxis() {
    PCA pca;
    pca.addMesh(obj);
    Pose res = pca.computeRotationAxis();

    axis = res.dir();
    r1 = res.x();
    r2 = res.up();
}

void VRThreadSegmentation::setBinSizes(double pe) {
    planeEps = pe;
}

void VRThreadSegmentation::setPitch(double p) { pitch = p; }

void VRThreadSegmentation::computePolarVertices() {
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
    midOffset = bb.center().dot(axis);

	for (auto p : pos) {
		PolarVertex v(p-axisOffset);
		v.computeAndSetAttributes(coords);
		axleVertices.push_back(v);
    }
}

void VRThreadSegmentation::analyse(VRObjectPtr o) {
    obj = o;
    if (!obj) return;

	auto same = [](double x, double y, double eps = 1) {
		return abs(x-y) < eps;
	};

	auto getVerticesOnSamePlane = [&]() {
		vector<vector<PolarVertex>> planes;
		planes.push_back(vector<PolarVertex>());
		int x = 0;

		vector<PolarVertex> vertices = axleVertices;
		std::sort(vertices.begin(), vertices.end(), [](const PolarVertex &a, const PolarVertex &b) { return a.profileCoords[0] < b.profileCoords[0]; });

		size_t size = vertices.size();
		for (int i=0; i<size-1; i++) {
			vertices[i].setPlaneIndex(x);
			planes[x].push_back(vertices[i]);

			bool b = same(vertices[i].profileCoords[0], vertices[i+1].profileCoords[0], planeEps);
			if (!b) {
                planes.push_back(vector<PolarVertex>());
				x++;
			}
		}

		auto& lastVertex = vertices[size-1];
		lastVertex.setPlaneIndex(x);
        planes[x].push_back(lastVertex);
		return planes;
    };

	auto getMinMaxRadius = [](vector<PolarVertex>& vertexList) {
		double rmin = 1e6;
		double rmax = -1e6;
		for (auto& v : vertexList) {
			rmin = min(rmin,v.radius);
			rmax = max(rmax,v.radius);
		}
		return Vec2d(rmin, rmax);
    };

	auto getTotalLength = [&]() {
		double pmin = 1e6;
		double pmax = -1e6;
		for (auto& v : axleVertices) {
            double x = v.profileCoords[0];
			pmin = min(pmin, x);
			pmax = max(pmax, x);
		}
		return pmax - pmin;
    };

	auto calcAxleParams = [&](vector<PolarVertex>& vertices) {
		double radius = 0;
		double offset = 0;
		double k = 1.0/vertices.size();
		for (auto& v : vertices) {
            radius += v.profileCoords[1]*k;
            offset += v.profileCoords[0]*k;
		}
		return vector<double>({radius, offset});
    };

    computeAxis();
    computePolarVertices();
    length = getTotalLength();
    radius = getMinMaxRadius(axleVertices)[1];

    vector<PolarVertex> outerVertices;
    for (PolarVertex& v : axleVertices) {
        if (same(v.radius,radius)) outerVertices.push_back(v);
    }
    axleVertices = outerVertices;

	vector<vector<PolarVertex>> planes = getVerticesOnSamePlane();
	for (auto& vertices : planes) {
		auto res = calcAxleParams(vertices);
		axleParams.push_back(res);
	}

	sineFit = SineFit::create();

    double di = axleParams[0][1];
    double dN = axleParams[axleParams.size()-1][1];
    double d = (dN - di) * 1.0/(axleParams.size()*8);

    for (unsigned int i=1; i<axleParams.size(); i++) {
        double a1 = axleParams[i-1][1];
        double r1 = axleParams[i-1][0];
        double a2 = axleParams[i][1];
        double r2 = axleParams[i][0];
        //cout << "" << Vec2d(r1, a1) << endl;
        while (di < a1) di += d;
        if (di > a2) continue;
        while (di < a2) {
            double Px = r1+(r2-r1)/(a2-a1)*(di-a1);
            sineFitInput.push_back(Vec2d(di,Px));
            di += d;
        }
    }

    /*for (float x=di; x<dN; x += d) { // test sine input, works perfectly :/
        double y = radius * sin(1.3*x);
        sineFitInput.push_back(Vec2d(x,y));
    }*/

    sineFit->vertices = sineFitInput;

    chosenFreqI = 1;

	pitch = 0.1;
    sineFit->compute(2);
	if (sineFit->fits.size() == 0) {
        cout << " Error! no sine fit results!" << endl;
        return;
	}
    vector<double> fitParams = sineFit->fits[chosenFreqI].fit;

    double f = fitParams[1];
	pitch = 2*Pi/f;
    cout << " freq " << f << " -> pitch " << pitch << endl;
}

VRGeometryPtr VRThreadSegmentation::createThread() {
    auto g = VRGeometry::create("thread");
    //g->setPrimitive("Cylinder "+toString(length)+" "+toString(radius));
    //g->setTransform(axisOffset + axis*midOffset, r1, axis);
    double teethLength = 0.5*pitch/tan(Pi/6);
    g->setPrimitive("Thread "+toString(length)+" "+toString(radius-0.5*teethLength)+" "+toString(pitch)+" 16");
    g->setTransform(axisOffset + axis*(midOffset-length*0.5), -axis, r1);
    return g;
}

VRTransformPtr VRThreadSegmentation::getProfileViz() {
    VRGeoData data;
	//for (auto& axleParam : axleParams) {
	    //Vec3d p = axisOffset + r1*axleParam[0] + axis*axleParam[1];
	for (auto& v : sineFitInput) {
	    Vec3d p = axisOffset + r1*v[1] + axis*v[0];
        data.pushVert(p);
        if (data.size() > 1) data.pushLine();
	}

	auto m = VRMaterial::create("threadProfViz");
	m->setLit(0);
	m->setLineWidth(2);
	m->setDiffuse(Color3f(1,0.3,0));
    auto geo = data.asGeometry("threadProfile");
    geo->setMaterial(m);
    return geo;
}

VRGeometryPtr VRThreadSegmentation::getSineFitViz(int precision) {
    auto m = VRMaterial::create("cMat");
    m->setLineWidth(2);
    m->setDiffuse(Color3f(0,1,0));
    m->setLit(0);

    if (!sineFit || sineFit->fits.size() <= chosenFreqI) return 0;
    vector<double> s = sineFit->fits[chosenFreqI].fit;

    VRGeoData curve;
    float l0 = axleParams[0][1];
    float dl = length/(precision-1);
    int j = precision;
    for (int i=0; i<precision; i++) {
        float x = l0+i*dl;
        float y = s[0] * sin(s[1]*x + s[2]) + s[3];
        Vec3d v = axis*x + axisOffset + r1*y;
        curve.pushVert(v);
        curve.pushLine(i, j%precision);
        j = i;
    }

    auto geo = curve.asGeometry("threadSineFit");
    geo->setMaterial(m);
    return geo;
}

vector<Vec2d> VRThreadSegmentation::getProfile() {
    vector<Vec2d> res;
	for (auto& axleParam : axleParams) res.push_back(Vec2d(axleParam[1], axleParam[0]));
	return res;
}

Vec3d VRThreadSegmentation::getAxis() { return axis; }
Vec3d VRThreadSegmentation::getAxisOffset() { return axisOffset; }
double VRThreadSegmentation::getRadius() { return radius; }
double VRThreadSegmentation::getLength() { return length; }

