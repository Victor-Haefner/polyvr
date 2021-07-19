#ifndef VRMULTIGRID_H_INCLUDED
#define VRMULTIGRID_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"

#include <OpenSG/OSGConfig.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMultiGrid : public VRGeometry {
	private:
        struct Grid {
            Vec4d rect;
            Vec2d res;
            int parent = -1;
            vector<int> children;
        };

        vector<Grid> grids;
        int outerGrid = -1;

        bool isInside(Grid& g1, Grid& g2);
        bool isInside(Grid& grid, const Vec3d& p);
        bool isInChild(Grid& grid, const Vec3d& p);

        bool checkForOverlap();
        void getOuterGrid();
        void computeTree();
        void computeGridGeo(int gridID, VRGeoData& data);
        void computeGeo(VRGeometryPtr geo = 0);

	public:
		VRMultiGrid(string name);
		~VRMultiGrid();

		static VRMultiGridPtr create(string name = "multiGrid");
		VRMultiGridPtr ptr();

		void addGrid(Vec4d rect, Vec2d res);
		bool compute(VRGeometryPtr geo = 0);
		void clear();
};

OSG_END_NAMESPACE;

#endif //VRMULTIGRID_H_INCLUDED
