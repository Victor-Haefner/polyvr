#ifndef VRUDPCLIENT_H_INCLUDED
#define VRUDPCLIENT_H_INCLUDED

#include "../VRNetworkClient.h"

class UDPClient;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRUDPClient : public VRNetworkClient {
	private:
        UDPClient* client = 0;

	public:
		VRUDPClient(string name);
		~VRUDPClient();

		static VRUDPClientPtr create(string name = "none");
		VRUDPClientPtr ptr();

        void onMessage( function<string(string)> f ) override;

        void connect(string host, int port) override;
        void send(const string& message, string guard = "", bool verbose = false) override;
};

OSG_END_NAMESPACE;

#endif //VRUDPCLIENT_H_INCLUDED
