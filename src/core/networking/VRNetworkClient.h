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
        string name = "none";
        string protocol = "none";
        string uri = "none";

	public:
		VRNetworkClient(string name);
		virtual ~VRNetworkClient();

        virtual void onMessage( function<string(string)> f ) {};
        virtual void connect(string host, int port) {};
        virtual void send(const string& message, string guard = "", bool verbose = false) {};

        string getName();
        string getProtocol();
        string getConnectedUri();
};

OSG_END_NAMESPACE;

#endif //VRNETWORKCLIENT_H_INCLUDED
