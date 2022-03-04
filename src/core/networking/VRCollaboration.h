#ifndef VRCOLLABORATION_H_INCLUDED
#define VRCOLLABORATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRNetworkingFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRCollaboration : public std::enable_shared_from_this<VRCollaboration> {
	private:
	public:
		VRCollaboration();
		~VRCollaboration();

		static VRCollaborationPtr create();
		VRCollaborationPtr ptr();
};

OSG_END_NAMESPACE;

#endif //VRCOLLABORATION_H_INCLUDED
