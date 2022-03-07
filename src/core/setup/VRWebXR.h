#ifndef VRWEBXR_H_INCLUDED
#define VRWEBXR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRSetupFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRWebXR : public std::enable_shared_from_this<VRWebXR> {
	private:
	public:
		VRWebXR();
		~VRWebXR();

		static VRWebXRPtr create();
		VRWebXRPtr ptr();
};

OSG_END_NAMESPACE;

#endif //VRWEBXR_H_INCLUDED
