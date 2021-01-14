#ifndef VRRESTSERVER_H_INCLUDED
#define VRRESTSERVER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VRNetworkingFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRestServer : public std::enable_shared_from_this<VRRestServer> {
	private:
	public:
		VRRestServer();
		~VRRestServer();

		static VRRestServerPtr create();
		VRRestServerPtr ptr();
};

OSG_END_NAMESPACE;

#endif //VRRESTSERVER_H_INCLUDED
