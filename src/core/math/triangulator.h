#ifndef TRIANGULATOR_H_INCLUDED
#define TRIANGULATOR_H_INCLUDED

#include "polygon.h"
#include "core/objects/VRObjectFwd.h"
#include <string>
#include <vector>

OSG_BEGIN_NAMESPACE;
using namespace std;

class Triangulator {
    public:
        VRGeoDataPtr geo;
        int num_points = 0;
        bool addNormals = true;
        vector<Vec3d> tmpVertices;

    private:
        vector<VRPolygon> outer_bounds;
        vector<VRPolygon> inner_bounds;

        void testQuad();
        void tessellate();

        vector<Vec3d> toSpace(const vector<Vec2d>& poly);

    public:
        Triangulator();
        ~Triangulator();
        static shared_ptr<Triangulator> create();

        void add(VRPolygon p, bool outer = true);

        size_t append(VRGeoDataPtr data, bool addNormals);
        VRGeometryPtr compute();
        VRGeometryPtr computeBounds();
};

OSG_END_NAMESPACE;

#endif // TRIANGULATOR_H_INCLUDED
