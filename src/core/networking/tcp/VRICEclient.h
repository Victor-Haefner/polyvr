#ifndef VRICECLIENT_H_INCLUDED
#define VRICECLIENT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VRNetworkingFwd.h"
#include "core/utils/VRFunctionFwd.h"

#include <map>
#include <vector>
#include <string>
#include <functional>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRICEClient : public std::enable_shared_from_this<VRICEClient> {
    public:
        enum CHANNEL {
            NONE = 0,
            SCENEGRAPH,
            AUDIO
        };

	private:
        VRRestClientPtr broker;
        map<string, map<CHANNEL, VRNetworkClientPtr> > clients;
        map<string, string> users;

        string name;
        string uID;
        string turnURL;
        string turnIP;
        bool usrGuard = false;
        bool msgGuard = false;

        function<void(string)> onEventCb;
        function<void(string)> onMessageCb;

        VRUpdateCbPtr updateCb;
        string usersList;
        void update();
        void updateUsers();

        void processUsers(string data);
        void processMessages(string data);
        void processRespUsers(VRRestResponsePtr r);
        void processRespMessages(VRRestResponsePtr r);

        void pollUsers(bool async);
        void pollMessages(bool async);

	public:
		VRICEClient();
		~VRICEClient();

		static VRICEClientPtr create();
		VRICEClientPtr ptr();

		void setTurnServer(string url);
        void onEvent( function<void(string)> f );
        void onMessage( function<void(string)> f );

        void setName(string name);
        void connectTo(string other);
        void sendTCP(string otherID, string msg, CHANNEL channel);
        void send(string otherID, string msg);
        void removeUser(string uid);

        string getID();
		string getUserName(string ID);
		vector<string> getUserID(string name);
		map<string, string> getUsers();
		VRNetworkClientPtr getClient(string otherID, CHANNEL channel);
		map<string, map<CHANNEL, VRNetworkClientPtr> > getClients();
};

OSG_END_NAMESPACE;

#endif //VRICECLIENT_H_INCLUDED
