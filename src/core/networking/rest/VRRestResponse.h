#ifndef VRRESTRESPONSE_H_INCLUDED
#define VRRESTRESPONSE_H_INCLUDED

#include <string>
#include <OpenSG/OSGConfig.h>
#include "../VRNetworkingFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRestResponse : public std::enable_shared_from_this<VRRestResponse> {
	private:
        int status = 0;
        string headers;
        string data;

	public:
		VRRestResponse();
		~VRRestResponse();

		static VRRestResponsePtr create();
		VRRestResponsePtr ptr();

		void setStatus(int s);
		void setHeaders(string s);
		void setData(string s);
		void appendData(string s);

		int getStatus();
		string getHeaders();
		string getData();

		static string uriEncode(const string& s);
};

OSG_END_NAMESPACE;

#endif //VRRESTRESPONSE_H_INCLUDED
