#ifndef VRICECLIENT_H_INCLUDED
#define VRICECLIENT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VRNetworkingFwd.h"

#include <map>
#include <vector>
#include <string>
#include <functional>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRICEClient : public std::enable_shared_from_this<VRICEClient> {
	private:
        VRRestClientPtr broker;
        VRTCPClientPtr client;
        map<string, string> users;

        string name;
        string uID;
        string turnURL;
        string turnIP;

        function<void(string)> onEventCb;
        function<void(string)> onMessageCb;

	public:
		VRICEClient();
		~VRICEClient();

		static VRICEClientPtr create();
		VRICEClientPtr ptr();

		void setTurnServer(string url, string ip);
        void onEvent( function<void(string)> f );
        void onMessage( function<void(string)> f );

        void setName(string name);
        void connectTo(string other);
        void send(string msg);
        void removeUser(string uid);

        string getID();
		string getUserName(string ID);
		vector<string> getUserID(string name);
		map<string, string> getUsers();
		VRTCPClientPtr getClient();
};

OSG_END_NAMESPACE;

#endif //VRICECLIENT_H_INCLUDED
