#ifndef VRRESTCLIENT_H_INCLUDED
#define VRRESTCLIENT_H_INCLUDED

#include <string>
#include <list>
#include <OpenSG/OSGConfig.h>
#include "../VRNetworkingFwd.h"

typedef void CURL;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRestClient : public std::enable_shared_from_this<VRRestClient> {
	private:
        struct RestPromise;

        bool isConnected = false;
        CURL* curl = 0;

        list<shared_ptr<RestPromise>> promises;

        void finishAsync(VRRestCbPtr cb, VRRestResponsePtr res);

	public:
		VRRestClient();
		~VRRestClient();

		static VRRestClientPtr create();
		VRRestClientPtr ptr();

		VRRestResponsePtr get(string uri, int timeoutSecs = 2);
		void getAsync(string uri, VRRestCbPtr cb, int timeoutSecs = 2);
		void post(string uri, const string& data, int timeoutSecs = 2);

		void connect(string uri, int timeoutSecs = 2);
		void connectPort(string uri, int port, int timeoutSecs = 2);
		bool connected();
		void post(const string& data);

		void test();
};

OSG_END_NAMESPACE;

#endif //VRRESTCLIENT_H_INCLUDED
