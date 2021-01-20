#ifndef VRRESTRESPONSE_H_INCLUDED
#define VRRESTRESPONSE_H_INCLUDED

#include <string>
#include <OpenSG/OSGConfig.h>
#include "../VRNetworkingFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRestResponse : public std::enable_shared_from_this<VRRestResponse> {
	private:
        string status;
        string data;

	public:
		VRRestResponse();
		~VRRestResponse();

		static VRRestResponsePtr create();
		VRRestResponsePtr ptr();

		void setStatus(string s);
		void setData(string s);
		void appendData(string s);

		string getStatus();
		string getData();
};

OSG_END_NAMESPACE;

#endif //VRRESTRESPONSE_H_INCLUDED
