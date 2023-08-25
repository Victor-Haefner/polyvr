#ifndef VRUDPSERVER_H_INCLUDED
#define VRUDPSERVER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VRNetworkServer.h"

#include <string>
#include <functional>

class UDPServer;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRUDPServer : public VRNetworkServer {
	private:
        UDPServer* server = 0;

	public:
		VRUDPServer(string name);
		~VRUDPServer();

		static VRUDPServerPtr create(string name = "none");

        void onMessage( function<string(string)> f, bool deferred = false );

		void listen(int port);
        void close();

        int getPort();
};

OSG_END_NAMESPACE;

#endif //VRUDPSERVER_H_INCLUDED
