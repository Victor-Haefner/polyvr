#include "CSGGeometry.h"
#include "CGALTypedefs.h"
#include "PolyhedronBuilder.h"

using namespace OSG;

void CSGGeometry::operate(CGALPolyhedron *P1, CGALPolyhedron *P2) {
    auto p1 = P1->polyhedron;
    auto p2 = P2->polyhedron;
	if (!p1->is_closed() || !p2->is_closed()) return;

    try {
        CGAL::Nef_Polyhedron np1(*p1), np2(*p2);
        if (operation == "unite") np1 += np2;
        else if(operation == "subtract") np1 -= np2;
        else if(operation == "intersect") np1 = np1.intersection(np2);
        else cout << "CSGGeometry: Warning: unexpected CSG operation!\n";
        np1.convert_to_polyhedron(*(polyhedron->polyhedron));
    } catch (exception e) { cout << getName() << ": CSGGeometry::operate exception: " << e.what() << endl; }
}
