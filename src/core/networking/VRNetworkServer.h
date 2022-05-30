#ifndef VRNETWORKSERVER_H_INCLUDED
#define VRNETWORKSERVER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRNetworkClient.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRNetworkServer : public std::enable_shared_from_this<VRNetworkServer> {
	protected:
	    string name;
	    string protocol;

        VRNetworkFlow inFlow;
        VRNetworkFlow outFlow;

	public:
		VRNetworkServer();
		virtual ~VRNetworkServer();

		static VRNetworkServerPtr create();
		VRNetworkServerPtr ptr();

		string getName();
		string getProtocol();

        VRNetworkFlow& getInFlow();
        VRNetworkFlow& getOutFlow();
};

OSG_END_NAMESPACE;

#endif //VRNETWORKSERVER_H_INCLUDED
