#include "VRAxleSegmentation.h"
#include "core/utils/toString.h"
#include "core/math/PCA.h"
#include "core/math/partitioning/boundingbox.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeoData.h"

using namespace OSG;

VRAxleSegmentation::VRAxleSegmentation() {}
VRAxleSegmentation::~VRAxleSegmentation() {}

VRAxleSegmentationPtr VRAxleSegmentation::create() { return VRAxleSegmentationPtr(new VRAxleSegmentation()); }

void VRAxleSegmentation::setBinSizes(double pe) {
    planeEps = pe;
}

void VRAxleSegmentation::computeAxis() {
    PCA pca;
    pca.addMesh(obj);
    Pose res = pca.computeRotationAxis();

    axis = res.dir();
    r1 = res.x();
    r2 = res.up();
}

void VRAxleSegmentation::computePolarVertices() {
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

void VRAxleSegmentation::analyse(VRObjectPtr o) {
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
			pmin = min(pmin, v.profileCoords[0]);
			pmax = max(pmax, v.profileCoords[0]);
		}
		return pmax - pmin;
    };

	auto calcAxleParams = [&](vector<PolarVertex>& vertices) {
		double radius = getMinMaxRadius(vertices)[1];
		double offset = vertices[0].profileCoords[0];
		return vector<double>({radius, offset});
    };

    computeAxis();
    computePolarVertices();
    length = getTotalLength();

	vector<vector<PolarVertex>> planes = getVerticesOnSamePlane();
	for (auto& vertices : planes) {
		auto res = calcAxleParams(vertices);
		axleParams.push_back(res);
		radius = max(radius, res[0]);
	}
}

VRGeometryPtr VRAxleSegmentation::createAxle() {
    auto g = VRGeometry::create("axle");
    g->setPrimitive("Cylinder "+toString(length)+" "+toString(radius));
    g->setTransform(axisOffset + axis*midOffset, r1, axis);
    return g;
}

VRTransformPtr VRAxleSegmentation::getProfileViz() {
    VRGeoData data;
	for (auto& axleParam : axleParams) {
	    Vec3d p = axisOffset + r1*axleParam[0] + axis*axleParam[1];
        data.pushVert(p);
        if (data.size() > 1) data.pushLine();
	}

	auto m = VRMaterial::create("axleProfViz");
	m->setLit(0);
	m->setLineWidth(2);
	m->setDiffuse(Color3f(1,0.3,0));
    auto geo = data.asGeometry("axleProfile");
    geo->setMaterial(m);
    return geo;
}

vector<Vec2d> VRAxleSegmentation::getProfile() {
    vector<Vec2d> res;
	for (auto& axleParam : axleParams) res.push_back(Vec2d(axleParam[1], axleParam[0]));
	return res;
}

Vec3d VRAxleSegmentation::getAxis() { return axis; }
Vec3d VRAxleSegmentation::getAxisOffset() { return axisOffset; }
double VRAxleSegmentation::getRadius() { return radius; }
double VRAxleSegmentation::getLength() { return length; }

