#ifndef VRRESTSERVER_H_INCLUDED
#define VRRESTSERVER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VRNetworkingFwd.h"

#include <string>
#include <vector>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRestServer : public std::enable_shared_from_this<VRRestServer> {
    public:
        struct Data;

        Data* getData();
		void onMessage(void* connection, VRRestResponsePtr msg);

	private:
	    string name;
        Data* data = 0;
        VRRestCbPtr callback;

	public:
		VRRestServer(string name);
		~VRRestServer();

		static VRRestServerPtr create(string name = "none");
		VRRestServerPtr ptr();

		void listen(int port, VRRestCbPtr cb);
        void sendString(void* connection, string data, int code = 200, vector<string> headers = {});
};

OSG_END_NAMESPACE;

#endif //VRRESTSERVER_H_INCLUDED
