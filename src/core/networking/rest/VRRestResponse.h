#ifndef VRRESTRESPONSE_H_INCLUDED
#define VRRESTRESPONSE_H_INCLUDED

#include <string>
#include <vector>
#include <OpenSG/OSGConfig.h>
#include "core/utils/VRFunctionFwd.h"
#include "../VRNetworkingFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRestResponse : public std::enable_shared_from_this<VRRestResponse> {
	private:
        int status = 0;
        vector<string> headers;
        string data;
        VRMessageCbPtr streamCb;

	public:
		VRRestResponse();
		~VRRestResponse();

		static VRRestResponsePtr create();
		VRRestResponsePtr ptr();

		void setStatus(int s);
		void setHeaders(vector<string> h);
		void appendHeader(string s);
		void setData(string s);
		void appendData(string s);
		void setStreamCb(VRMessageCbPtr cb);

		int getStatus();
		vector<string> getHeaders();
		string getData();
		VRMessageCbPtr getStreamCb();

		static string uriEncode(const string& s);
};

OSG_END_NAMESPACE;

#endif //VRRESTRESPONSE_H_INCLUDED
