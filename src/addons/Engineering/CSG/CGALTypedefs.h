/*
 * CgalTypedefs.h
 *
 *  Created on: Jan 12, 2014
 *      Author: marcel
 *
 *  CGAL uses loads of template classes. It seems to be common practice to typedef the needed classes
 *  to have shorter && easier names. This way, it might also be possible to switch to a more precise
 *  kernel "easily".
 *  Predefining most CGAL types used here as they need exactly these template arguments anyway.
 *  Besides, the actual code is shortened (but stays readable).
 */

#ifndef CGALTYPEDEFS_H_
#define CGALTYPEDEFS_H_

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/Polyhedron_items_with_id_3.h>

namespace CGAL {
    typedef Exact_predicates_exact_constructions_kernel Kernel;	// Mostly referenced as "Epeck"
    class Polyhedron : public Polyhedron_3<Kernel> {};
    //typedef Polyhedron_3<Kernel/*, Polyhedron_items_with_id_3*/> Polyhedron; // Item "with_id_3" is necessary for vertex indices
    typedef Polyhedron::Vertex_iterator	Vertex_iterator;
    typedef Polyhedron::HalfedgeDS		HalfedgeDS;
    typedef Nef_polyhedron_3<Kernel>	Nef_Polyhedron;
    typedef Vector_3<Kernel>			Vector;
    typedef Point_3<Kernel>				Point;
    typedef Aff_transformation_3<Kernel> Transformation;
}


#endif /* CGALTYPEDEFS_H_ */
