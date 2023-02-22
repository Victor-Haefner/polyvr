#ifndef VRRESTCLIENT_H_INCLUDED
#define VRRESTCLIENT_H_INCLUDED

#include <string>
#include <list>
#include <OpenSG/OSGConfig.h>
#include "../VRNetworkClient.h"

typedef void CURL;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRestClient : public VRNetworkClient {
	private:
        struct RestPromise;

        bool isConnected = false;
        CURL* curl = 0;

        list<shared_ptr<RestPromise>> promises;

        void finishAsync(VRRestCbPtr cb, VRRestResponsePtr res);

	public:
		VRRestClient(string name);
		~VRRestClient();

		static VRRestClientPtr create(string name = "none");
		//VRRestClientPtr ptr();

		VRRestResponsePtr get(string uri, int timeoutSecs = 2);
		void getAsync(string uri, VRRestCbPtr cb, int timeoutSecs = 2);
		void post(string uri, const string& data, int timeoutSecs = 2);

		void connect(string uri, int timeoutSecs = 2);
		void connectPort(string uri, int port, int timeoutSecs = 2);
		bool connected();

		void test();
};

OSG_END_NAMESPACE;

#endif //VRRESTCLIENT_H_INCLUDED
