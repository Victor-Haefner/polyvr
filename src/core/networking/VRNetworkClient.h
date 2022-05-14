#ifndef VRNETWORKCLIENT_H_INCLUDED
#define VRNETWORKCLIENT_H_INCLUDED

#include "VRNetworkingFwd.h"

#include <OpenSG/OSGConfig.h>
#include <string>
#include <vector>
#include <functional>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRNetworkFlow {
    private:
        double kbs = 0;
        vector<double> kbs_queue;
        int kbs_queue_ptr = 0;

    public:
        VRNetworkFlow();

        void logFlow(double kb);
        double getKBperSec();
};

class VRNetworkClient : public std::enable_shared_from_this<VRNetworkClient> {
	private:
        VRNetworkFlow inFlow;
        VRNetworkFlow outFlow;

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

        VRNetworkFlow& getInFlow();
        VRNetworkFlow& getOutFlow();
};

OSG_END_NAMESPACE;

#endif //VRNETWORKCLIENT_H_INCLUDED
