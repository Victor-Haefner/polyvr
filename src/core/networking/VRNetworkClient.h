#ifndef VRNETWORKCLIENT_H_INCLUDED
#define VRNETWORKCLIENT_H_INCLUDED

#include "VRNetworkingFwd.h"

#include <OpenSG/OSGConfig.h>
#include <string>
#include <functional>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRNetworkClient {
	private:

    protected:
        string protocol = "none";

	public:
		VRNetworkClient();
		virtual ~VRNetworkClient();

        virtual void onMessage( function<string(string)> f ) {};
        virtual void connect(string host, int port) {};
        virtual void send(const string& message, string guard = "", bool verbose = false) {};

        string getProtocol();
};

OSG_END_NAMESPACE;

#endif //VRNETWORKCLIENT_H_INCLUDED
