#ifndef VRKABSCHALGORITHM_H_INCLUDED
#define VRKABSCHALGORITHM_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>
#include "VRMathFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRKabschAlgorithm : public std::enable_shared_from_this<VRKabschAlgorithm> {
	private:
        vector<Vec3d> points1;
        vector<Vec3d> points2;
        vector<Vec2i> matches;

        Vec3d centroid(vector<Vec3d> pnts);

	public:
		VRKabschAlgorithm();
		~VRKabschAlgorithm();

		static VRKabschAlgorithmPtr create();
		VRKabschAlgorithmPtr ptr();

        void setPoints1( vector<Vec3d>& pnts );
        void setPoints2( vector<Vec3d>& pnts );
        void setMatches( vector<Vec2i>& mths );

        void setSimpleMatches();

        Matrix4d compute(bool verbose = false);

        static void test();
};

OSG_END_NAMESPACE;

#endif //VRKABSCHALGORITHM_H_INCLUDED
