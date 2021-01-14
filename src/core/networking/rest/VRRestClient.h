#ifndef VRRESTCLIENT_H_INCLUDED
#define VRRESTCLIENT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VRNetworkingFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRestClient : public std::enable_shared_from_this<VRRestClient> {
	private:
	public:
		VRRestClient();
		~VRRestClient();

		static VRRestClientPtr create();
		VRRestClientPtr ptr();

		VRRestResponsePtr get(string uri, int timeoutSecs = 2);

		void test();
};

OSG_END_NAMESPACE;

#endif //VRRESTCLIENT_H_INCLUDED
