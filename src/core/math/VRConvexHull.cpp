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

    for (auto it = poly.facets_begin(); it != poly.facets_end(); it++) {
        for (auto fitr = it->facet_begin(); fitr != it->facet_begin(); fitr++) {
			cgalPoint cp = fitr->vertex()->point();
			Pnt3f p = Pnt3f(cp.x(), cp.y(), cp.z());
			data.pushVert(p, Vec3f(0,1,0));
		}
		if (it->is_triangle()) data.pushTri();
		if (it->is_quad())     data.pushQuad();
	}


	return data.asGeometry(name);
}

VRGeometryPtr VRConvexHull::compute3D(VRGeometryPtr geo) {
    vector<cgalPoint> points;
    auto pos = geo->getMesh()->geo->getPositions();
    for (int i=0; i<pos->size(); i++) {
        Pnt3f p = pos->getValue<Pnt3f>(i);
        points.push_back(cgalPoint(p[0], p[1], p[2]));
    }

	cout << "compute3D " << points.size() << endl;

    Polyhedron poly; // define polyhedron to hold convex hull
    CGAL::convex_hull_3(points.begin(), points.end(), poly); // compute convex hull of non-collinear points
    return toGeometry(poly, "convexHull");
}





