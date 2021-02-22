#ifndef VRPIPESYSTEM_H_INCLUDED
#define VRPIPESYSTEM_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VREngineeringFwd.h"
#include "core/math/VRMathFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRPipeSystem : public std::enable_shared_from_this<VRPipeSystem> {
	private:
        GraphPtr graph;

        map<int, VRPipeNodePtr> nodes;
        map<int, VRPipeSegmentPtr> segments;

	public:
		VRPipeSystem();
		~VRPipeSystem();

		static VRPipeSystemPtr create();
		VRPipeSystemPtr ptr();
};

OSG_END_NAMESPACE;

#endif //VRPIPESYSTEM_H_INCLUDED
