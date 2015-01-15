#ifndef POLYHEDRONBUILDER_H_
#define POLYHEDRONBUILDER_H_

#include <CGAL/Polyhedron_incremental_builder_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>

// A modifier creating a triangle with the incremental builder.
// Source: http://jamesgregson.blogspot.de/2012/05/example-code-for-building.html
template<class HDS>
class PolyhedronBuilder : public CGAL::Modifier_base<HDS> {
    public:
        std::vector<CGAL::Point> &coords;
        std::vector<size_t> &indices;

        PolyhedronBuilder(std::vector<CGAL::Point>& c, std::vector<size_t>& i) :
            coords(c), indices(i)
        {}

        void operator()(HDS& hds) {
            // create a cgal incremental builder
            CGAL::Polyhedron_incremental_builder_3<HDS> builder(hds, true);
            builder.begin_surface(coords.size()/3, indices.size()/3, 0, CGAL::Polyhedron_incremental_builder_3<HDS>::ABSOLUTE_INDEXING);

            for(auto c : coords) builder.add_vertex(c);

            // add the polyhedron triangles
            for(size_t i = 0; i < indices.size(); i += 3) {
                builder.begin_facet();
                builder.add_vertex_to_facet(indices[i+0]);
                builder.add_vertex_to_facet(indices[i+1]);
                builder.add_vertex_to_facet(indices[i+2]);
                builder.end_facet();
            }

            // finish up the surface (triangle)
            builder.end_surface();
        }
};

#endif /* POLYHEDRONBUILDER_H_ */
