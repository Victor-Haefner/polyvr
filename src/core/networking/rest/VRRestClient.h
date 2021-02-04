#ifndef VRRESTCLIENT_H_INCLUDED
#define VRRESTCLIENT_H_INCLUDED

#include <string>
#include <list>
#include <OpenSG/OSGConfig.h>
#include "../VRNetworkingFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRestClient : public std::enable_shared_from_this<VRRestClient> {
	private:
        struct RestPromise;

        list<shared_ptr<RestPromise>> promises;

        void finishAsync(VRRestCbPtr cb, VRRestResponsePtr res);

	public:
		VRRestClient();
		~VRRestClient();

		static VRRestClientPtr create();
		VRRestClientPtr ptr();

		VRRestResponsePtr get(string uri, int timeoutSecs = 2);
		void getAsync(string uri, VRRestCbPtr cb, int timeoutSecs = 2);

		void test();
};

OSG_END_NAMESPACE;

#endif //VRRESTCLIENT_H_INCLUDED
