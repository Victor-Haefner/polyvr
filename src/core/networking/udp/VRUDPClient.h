#ifndef VRUDPCLIENT_H_INCLUDED
#define VRUDPCLIENT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VRNetworkingFwd.h"

class UDPClient;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRUDPClient {
	private:
        string uri;
        UDPClient* client = 0;

	public:
		VRUDPClient();
		~VRUDPClient();

		static VRUDPClientPtr create();
		VRUDPClientPtr ptr();

        void connect(string host, int port);
        void send(const string& message, bool verbose = false);
};

OSG_END_NAMESPACE;

#endif //VRUDPCLIENT_H_INCLUDED
