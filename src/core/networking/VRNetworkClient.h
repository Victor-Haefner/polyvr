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
        vector<double> kbps;
        vector<pair<long long, double>> kb_queue;
        int kb_queue_ptr = 0;
        int Nq = 1;
        int Nv = 1;

        void updateKBpSec();

    public:
        VRNetworkFlow();

        void logFlow(double kb);
        vector<double>& getKBperSec();
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
        virtual bool isConnected(string host, int port) { return false; };
        virtual void send(const string& message, string guard = "", bool verbose = false) {};

        string getName();
        string getProtocol();
        string getConnectedUri();

        VRNetworkFlow& getInFlow();
        VRNetworkFlow& getOutFlow();
};

OSG_END_NAMESPACE;

#endif //VRNETWORKCLIENT_H_INCLUDED
