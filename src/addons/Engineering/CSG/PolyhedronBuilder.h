/*
 * PolyhedronBuilder.h
 *
 *  Created on: Jan 22, 2014
 *      Author: marcel
 */

#ifndef POLYHEDRONBUILDER_H_
#define POLYHEDRONBUILDER_H_

#include <CGAL/Polyhedron_incremental_builder_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>

// A modifier creating a triangle with the incremental builder.
// Source: http://jamesgregson.blogspot.de/2012/05/example-code-for-building.html
template<class HDS>
class PolyhedronBuilder : public CGAL::Modifier_base<HDS>
{
public:
	std::vector<CGAL::Point> &_coords;
	std::vector<size_t> &_indices;

	PolyhedronBuilder(std::vector<CGAL::Point> &coords, std::vector<size_t> &indices) :
		_coords(coords),
		_indices(indices)
	{
		// Make sure this results in at least one triangle
		assert(coords.size() > 2);
		assert(indices.size() > 2);
	}

	void operator()(HDS& hds)
	{
		//typedef typename HDS::Vertex   Vertex;
//		typedef typename Vertex::Point Point;

		// create a cgal incremental builder
		CGAL::Polyhedron_incremental_builder_3<HDS> builder(hds, true);
		builder.begin_surface(_coords.size()/3, _indices.size()/3, 0, CGAL::Polyhedron_incremental_builder_3<HDS>::ABSOLUTE_INDEXING);

//		std::printf("coords: %i\n", _coords.size());
//		for(int i = 0; i < _indices.size(); i++)
//			std::printf("%i: %i\n", i, _indices[i]);

		// add the polyhedron vertices
//		for(int i = 0; i < (int)_coords.size(); i += 3)
//			builder.add_vertex(Point(_coords[i+0], _coords[i+1], _coords[i+2]));

		for(std::vector<CGAL::Point>::iterator it = _coords.begin(); it != _coords.end(); it++)
		{
			builder.add_vertex(*it);
//			printf("added %f %f %f\n", it->x(), it->y(), it->z());
		}

		// add the polyhedron triangles
		for(size_t i = 0; i < _indices.size(); i += 3)
		{
//			std::printf("f: %u i: %u\n", i/3, i);
			builder.begin_facet();
			builder.add_vertex_to_facet(_indices[i+0]);
			builder.add_vertex_to_facet(_indices[i+1]);
			builder.add_vertex_to_facet(_indices[i+2]);
//			builder.add_vertex_to_facet(_indices[i+3]);
			builder.end_facet();
		}

		// finish up the surface (triangle)
		builder.end_surface();
	}
};

#endif /* POLYHEDRONBUILDER_H_ */
