#ifndef VRUDPSERVER_H_INCLUDED
#define VRUDPSERVER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VRNetworkingFwd.h"

#include <functional>

class UDPServer;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRUDPServer {
	private:
        UDPServer* server = 0;
        int port = 0;

	public:
		VRUDPServer();
		~VRUDPServer();

		static VRUDPServerPtr create();

        void onMessage( function<void(string)> f );

		void listen(int port);
        void close();

        int getPort();
};

OSG_END_NAMESPACE;

#endif //VRUDPSERVER_H_INCLUDED
