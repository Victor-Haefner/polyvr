#include "VRConvexHull.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeoData.h"

using namespace OSG;

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/algorithm.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/convex_hull_3.h>
#include <vector>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Polyhedron_3<K> Polyhedron;
typedef K::Point_3 cgalPoint;

VRConvexHull::VRConvexHull() {}
VRConvexHull::~VRConvexHull() {}

VRGeometryPtr toGeometry(Polyhedron& poly, string name) {
    VRGeoData data;

    int i=0;
    map<void*, int> points;
    for (auto face = poly.facets_begin(); face != poly.facets_end(); face++) {
        auto vertex = face->facet_begin();
        vector<int> faceIndx;
        do {
			auto v = (void*)&(*vertex->vertex());
			//auto v = (void*)vertex->vertex();
			if (!points.count(v)) {
                points[v] = i; i++;
                cgalPoint cp = vertex->vertex()->point();
                Pnt3d p = Pnt3d(cp.x(), cp.y(), cp.z());
                data.pushVert(p, Vec3d(1,0,0) );
            }
            faceIndx.push_back( points[v] );
		} while (++vertex != face->facet_begin());
		if (face->is_triangle()) data.pushTri(faceIndx[0], faceIndx[1], faceIndx[2]);
		if (face->is_quad())     data.pushQuad(faceIndx[0], faceIndx[1], faceIndx[2], faceIndx[3]);
	}

	auto geo = data.asGeometry(name);
	geo->updateNormals();
	return geo;
}

VRGeometryPtr VRConvexHull::compute(VRGeometryPtr geo) {
    if (!geo) return 0;
    vector<cgalPoint> points;
    if (!geo->getMesh()) return 0;
    if (!geo->getMesh()->geo) return 0;
    auto pos = geo->getMesh()->geo->getPositions();
    if (!pos) return 0;

    for (unsigned int i=0; i<pos->size(); i++) {
        Pnt3d p = Pnt3d(pos->getValue<Pnt3f>(i));
        points.push_back(cgalPoint(p[0], p[1], p[2]));
    }

    if (points.size() < 4) return 0;

    VRGeometryPtr res = 0;
    try { // TODO: check if points bounding box is not size 0!!!
        Polyhedron poly; // define polyhedron to hold convex hull
        CGAL::convex_hull_3(points.begin(), points.end(), poly); // compute convex hull of non-collinear points
        res = toGeometry(poly, "convexHull");
    } catch( exception e ) { cout << "ARGH\n" << e.what() << endl; res = 0; }
    return res;
}





