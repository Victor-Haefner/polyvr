#ifndef VRLADENGINE_H_INCLUDED
#define VRLADENGINE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/Engineering/VREngineeringFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRLADEngine : public std::enable_shared_from_this<VRLADEngine> {
	private:
	public:
		VRLADEngine();
		~VRLADEngine();

		static VRLADEnginePtr create();
		VRLADEnginePtr ptr();

		void read();
		void iterate();
};

OSG_END_NAMESPACE;

#endif //VRLADENGINE_H_INCLUDED
